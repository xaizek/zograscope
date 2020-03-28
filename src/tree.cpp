// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "tree.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/functional/hash.hpp>
#include <boost/utility/string_ref.hpp>

#include <cassert>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/strings.hpp"
#include "utils/trees.hpp"
#include "Language.hpp"
#include "STree.hpp"
#include "TreeBuilder.hpp"
#include "decoration.hpp"
#include "types.hpp"

// XXX: lexer also has such variable and they need to be synchronized
//      (actually we should pass this to lexer).
constexpr int tabWidth = 4;

static void putNodeChild(Node &parent, Node *child, const Language *lang);
static void preStringifyPTree(const std::string &contents,
                              PNode *node, const Language *lang,
                              cpp17::pmr::string &stringified);
static boost::string_ref stringifyPNode(const cpp17::pmr::string &stringified,
                                        const PNode *node);
static void preStringifyPNode(const std::string &contents, PNode *node,
                              const Language *lang,
                              cpp17::pmr::string &stringified);
static std::string stringifyPNodeSpelling(const std::string &contents,
                                          const PNode *node);
static int maxStringifiedSize(boost::string_ref contents);
static void postOrder(Node &node, std::vector<Node *> &v);
static std::vector<std::size_t> hashChildren(const Node &node);
static std::size_t hashNode(const Node *node);
static void markAsMoved(Node *node, Language &lang);
static void dumpTree(std::ostream &os, const Node *node, const Language *lang,
                     std::vector<bool> &trace, int depth);
static void dumpNode(std::ostream &os, const Node *node, const Language *lang);

Tree::Tree(std::unique_ptr<Language> lang, const std::string &contents,
           const PNode *node, allocator_type al)
    : lang(std::move(lang)), nodes(al), stringified(al), internPool(al)
{
    stringified.reserve(maxStringifiedSize(contents));
    const char *buf = stringified.c_str();

    preStringifyPTree(contents, const_cast<PNode *>(node), this->lang.get(),
                      stringified);
    root = materializePNode(contents, node);

    assert(stringified.c_str() == buf && "Stringified buffer got relocated!");
    (void)buf;
}

Tree::Tree(std::unique_ptr<Language> lang, const std::string &contents,
           const SNode *node, allocator_type al)
    : lang(std::move(lang)), nodes(al), stringified(al), internPool(al)
{
    stringified.reserve(maxStringifiedSize(contents));
    const char *buf = stringified.c_str();

    preStringifyPTree(contents, node->value, this->lang.get(), stringified);
    root = materializeSNode(contents, node, nullptr);

    assert(stringified.c_str() == buf && "Stringified buffer got relocated!");
    (void)buf;
}

Node *
Tree::materializeSNode(const std::string &contents, const SNode *node,
                       const SNode *parent)
{
    Node &n = *nodes.make();
    n.stype = node->value->stype;
    n.satellite = lang->isSatellite(n.stype);

    if (node->children.empty()) {
        const PNode *leftmostLeaf = node->value->leftmostChild();

        n.label = stringifyPNode(stringified, node->value);
        n.line = leftmostLeaf->line;
        n.col = leftmostLeaf->col;
        n.next = materializePNode(contents, node->value);
        n.next->last = true;
        n.type = n.next->type;
        n.leaf = (n.line != 0 && n.col != 0);
        return &n;
    }

    n.children.reserve(node->children.size());
    for (SNode *child : node->children) {
        Node *newChild = materializeSNode(contents, child, node);
        putNodeChild(n, newChild, lang.get());
    }

    // The check below can be true if putNodeChild() decided to not add any
    // children.
    if (!n.children.empty()) {
        n.line = n.children.front()->line;
        n.col = n.children.front()->col;
    }

    auto valueChild = std::find_if(node->children.begin(), node->children.end(),
                                   [this](const SNode *node) {
                                       const SType stype = node->value->stype;
                                       return lang->isValueNode(stype);
                                   });
    if (valueChild != node->children.end()) {
        n.label = stringifyPNode(stringified, (*valueChild)->value);
        n.valueChild = valueChild - node->children.begin();
    } else {
        n.valueChild = -1;
    }

    // Move certain nodes onto the next layer.
    SType parentSType = (parent == nullptr ? SType{} : parent->value->stype);
    if (lang->isLayerBreak(parentSType, n.stype)) {
        Node &nextLevel = *nodes.make();
        nextLevel.next = &n;
        nextLevel.stype = n.stype;
        nextLevel.line = n.line;
        nextLevel.col = n.col;

        int len = node->value->value.postponedTo;
        nextLevel.label = n.label.empty() ? intern(printSubTree(n, false, len))
                                          : n.label;
        return &nextLevel;
    }

    return &n;
}

// Adds child or its children (when child is spliced) to the parent node.
static void
putNodeChild(Node &parent, Node *child, const Language *lang)
{
    if (!lang->shouldSplice(parent.stype, child)) {
        parent.children.emplace_back(child);
        return;
    }

    if (child->next != nullptr) {
        // Make sure we don't splice last layer.
        if (child->next->last) {
            // Unless it's empty (has neither children nor value).
            if (!child->next->children.empty() ||
                !child->next->label.empty()) {
                parent.children.emplace_back(child);
            }
            return;
        }

        child = child->next;
    }

    for (auto x : child->children) {
        putNodeChild(parent, x, lang);
    }
}

// Turns tree into a string.  Stores boundaries of nodes label in
// value.postponedFrom (start index) and value.postponedTo (length).
static void
preStringifyPTree(const std::string &contents, PNode *node,
                  const Language *lang, cpp17::pmr::string &stringified)
{
    struct {
        const std::string &contents;
        const Language *lang;
        cpp17::pmr::string &out;
        void run(PNode *node)
        {
            node->value.postponedFrom = out.size();

            if (node->line != 0 && node->col != 0) {
                preStringifyPNode(contents, node, lang, out);
            }

            for (PNode *child : node->children) {
                run(child);
            }

            if (node->line == 0 || node->col == 0) {
                node->value.postponedTo = out.size()
                                        - node->value.postponedFrom;
            }
        }
    } visitor { contents, lang, stringified };

    visitor.run(node);
}

Node *
Tree::materializePNode(const std::string &contents, const PNode *node)
{
    const Type type = lang->mapToken(node->value.token);

    if (type == Type::Virtual && node->children.size() == 1U) {
        return materializePNode(contents, node->children[0]);
    }

    Node &n = *nodes.make();
    n.label = stringifyPNode(stringified, node);
    if (lang->shouldDropLeadingWS(node->stype)) {
        n.spelling = intern(stringifyPNodeSpelling(contents, node));
    } else {
        n.spelling = n.label;
    }
    n.line = node->line;
    n.col = node->col;
    n.type = type;
    n.stype = node->stype;
    n.leaf = (n.line != 0 && n.col != 0);

    n.children.reserve(node->children.size());
    for (const PNode *child : node->children) {
        n.children.push_back(materializePNode(contents, child));
    }

    return &n;
}

// Turns PNode into a string.
static boost::string_ref
stringifyPNode(const cpp17::pmr::string &stringified, const PNode *node)
{
    return boost::string_ref(stringified.c_str() + node->value.postponedFrom,
                             node->value.postponedTo);
}

// Turns node into a string.  Stores boundaries of this node's label in
// value.postponedFrom (start index) and value.postponedTo (length).
static void
preStringifyPNode(const std::string &contents, PNode *node,
                  const Language *lang, cpp17::pmr::string &stringified)
{
    node->value.postponedFrom = stringified.size();

    bool leadingWhitespace = false;
    int col = node->col;
    for (std::size_t i = node->value.from;
         i < node->value.from + node->value.len; ++i) {
        switch (const char c = contents[i]) {
            int width;

            case '\n':
                col = 1;
                stringified += '\n';
                leadingWhitespace = lang->shouldDropLeadingWS(node->stype);
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                col += width;
                if (!leadingWhitespace) {
                    stringified.append(width, ' ');
                }
                break;
            case ' ':
                ++col;
                if (!leadingWhitespace) {
                    stringified += ' ';
                }
                break;

            default:
                ++col;
                stringified += c;
                leadingWhitespace = false;
                break;
        }
    }

    node->value.postponedTo = stringified.size() - node->value.postponedFrom;
}

// Computes node label only expanding tabs in it.
static std::string
stringifyPNodeSpelling(const std::string &contents, const PNode *node)
{
    boost::string_ref sr(contents.c_str() + node->value.from, node->value.len);

    std::string str;
    str.reserve(maxStringifiedSize(sr));

    int col = node->col;
    for (char c : sr) {
        switch (c) {
            int width;

            case '\n':
                col = 1;
                str += '\n';
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                col += width;
                str.append(width, ' ');
                break;

            default:
                ++col;
                str += c;
                break;
        }
    }
    return str;
}

// Computes maximum length of stringified buffer.
static int
maxStringifiedSize(boost::string_ref contents)
{
    static_assert(tabWidth > 0, "Tabulation can't have zero width.");

    int nTabs = std::count(contents.cbegin(), contents.cend(), '\t');
    return contents.size() + nTabs*(tabWidth - 1);
}

std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    root.parent = &root;
    postOrder(root, v);
    return v;
}

static void
postOrder(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    for (Node *child : node.children) {
        child->parent = &node;
        postOrder(*child, v);
    }
    node.poID = v.size();
    v.push_back(&node);
}

void
reduceTreesCoarse(Node *T1, Node *T2)
{
    struct {
        void match(Node *x, Node *y) {
            // Assumption is that matched nodes have exactly the same structure.
            // Might want to check it in code.

            x->state = State::Unchanged;
            y->state = State::Unchanged;
            x->relative = y;
            y->relative = x;

            for (auto l = x->children.begin(), r = y->children.begin();
                 l != x->children.end() && r != y->children.end();
                 ++l, ++r) {
                match(*l, *r);
            }

            if (x->next && !x->next->last && y->next && !y->next->last) {
                match(x->next, y->next);
            }
        }
    } matcher;

    const std::vector<std::size_t> children1 = hashChildren(*T1);
    const std::vector<std::size_t> children2 = hashChildren(*T2);

    for (unsigned int i = 0U, n = children1.size(); i < n; ++i) {
        const std::size_t hash1 = children1[i];
        for (unsigned int j = 0U, n = children2.size(); j < n; ++j) {
            if (T2->children[j]->satellite) {
                continue;
            }

            const std::size_t hash2 = children2[j];
            if (hash1 == hash2) {
                matcher.match(T1->children[i], T2->children[j]);
                T1->children[i]->satellite = true;
                T2->children[j]->satellite = true;
                break;
            }
        }
    }
}

// Hashes direct children of the node individually.
static std::vector<std::size_t>
hashChildren(const Node &node)
{
    std::vector<std::size_t> hashes;
    hashes.reserve(node.children.size());
    for (const Node *child : node.children) {
        hashes.push_back(hashNode(child));
    }
    return hashes;
}

// Hashes all descendants of the node together.
static std::size_t
hashNode(const Node *node)
{
    if (node->next != nullptr) {
        return hashNode(node->next);
    }

    std::size_t hash = boost::hash_range(node->label.begin(),
                                         node->label.end());
    for (const Node *child : node->children) {
        boost::hash_combine(hash, hashNode(child));
    }
    return hash;
}

std::string
printSubTree(const Node &root, bool withComments, int size_hint)
{
    struct {
        bool withComments;
        std::string out;
        void run(const Node &node)
        {
            if (node.next != nullptr) {
                return run(*node.next);
            }

            if (node.leaf && (node.type != Type::Comments || withComments)) {
                out.append(node.label.cbegin(), node.label.cend());
            }

            for (const Node *child : node.children) {
                run(*child);
            }
        }
    } visitor { withComments, {} };

    if (size_hint > 0) {
        visitor.out.reserve(size_hint);
    }
    visitor.run(root);

    return visitor.out;
}

bool
canForceLeafMatch(const Node *x, const Node *y)
{
    if (!x->children.empty() || !y->children.empty()) {
        return false;
    }

    const Type xType = canonizeType(x->type);
    const Type yType = canonizeType(y->type);
    return xType == yType
        && xType != Type::Virtual
        && xType != Type::Comments
        && xType != Type::Identifiers
        && xType != Type::Directives;
}

void
Tree::markTreeAsMoved(Node *node)
{
    if (lang->hasMoveableItems(node)) {
        markAsMoved(node, *lang);
    }
}

// Marks all movable nodes in the subtree as moved.
static void
markAsMoved(Node *node, Language &lang)
{
    node->moved = !lang.isUnmovable(node);

    for (Node *child : node->children) {
        markAsMoved(child, lang);
    }
}

void
Tree::dump() const
{
    if (root != nullptr) {
        dumpTree(std::cout, root, lang.get());
    }
}

void
dumpTree(std::ostream &os, const Node *node, const Language *lang)
{
    std::vector<bool> trace;
    dumpTree(os, node, lang, trace, 0);
}

// Dumps subtree onto standard output.
static void
dumpTree(std::ostream &os, const Node *node, const Language *lang,
         std::vector<bool> &trace, int depth)
{
    using namespace decor;
    using namespace decor::literals;

    Decoration sepHi = 246_fg;
    Decoration depthHi = 250_fg;

    os << sepHi;

    os << (trace.empty() ? "--- " : "    ");

    for (unsigned int i = 0U, n = trace.size(); i < n; ++i) {
        bool last = (i == n - 1U);
        if (trace[i]) {
            os << (last ? "`-- " : "    ");
        } else {
            os << (last ? "|-- " : "|   ");
        }
    }

    os << def;

    os << (depthHi << depth) << (sepHi << " | ");
    dumpNode(os, node, lang);

    trace.push_back(false);
    for (unsigned int i = 0U, n = node->children.size(); i < n; ++i) {
        Node *child = node->children[i];

        trace.back() = (i == n - 1U);
        dumpTree(os, child, lang, trace, depth);

        if (child->next != nullptr && !child->next->last) {
            trace.push_back(true);
            dumpTree(os, child->next, lang, trace, depth + 1);
            trace.pop_back();
        }
    }
    trace.pop_back();
}

// Dumps single node into a stream.
static void
dumpNode(std::ostream &os, const Node *node, const Language *lang)
{
    using namespace decor;
    using namespace decor::literals;

    Decoration labelHi = 78_fg + bold;
    Decoration relLabelHi = 78_fg;
    Decoration idHi = bold;
    Decoration movedHi = 33_fg + inv + bold + 231_bg;
    Decoration insHi = 82_fg + inv + bold;
    Decoration updHi = 226_fg + inv + bold;
    Decoration delHi = 160_fg + inv + bold + 231_bg;
    Decoration relHi = 226_fg + bold;
    Decoration typeHi = 51_fg;
    Decoration stypeHi = 222_fg;

    auto label = [](boost::string_ref sr) {
        std::string str;
        str.reserve(1 + sr.size() + 1);
        str += '`';
        str.append(sr.begin(), sr.end());
        str += '`';
        boost::replace_all(str, "\n", "<NL>");
        return str;
    };

    if (node->moved) {
        os << (movedHi << '!');
    }

    switch (node->state) {
        case State::Unchanged: break;
        case State::Deleted:  os << (delHi << '-'); break;
        case State::Inserted: os << (insHi << '+'); break;
        case State::Updated:  os << (updHi << '~'); break;
    }

    os << (labelHi << label(node->label))
       << (idHi << " #" << node->poID);

    os << (node->satellite ? ", Satellite" : "") << ", "
       << (typeHi << "Type::" << node->type) << ", "
       << (stypeHi << lang->toString(node->stype));

    if (node->relative != nullptr) {
        os << (relHi << " -> ") << (relLabelHi << label(node->relative->label))
           << (idHi << " #" << node->relative->poID);
    }

    os << '\n';
}

void
Tree::propagateStates()
{
    std::function<void(Node &, State)> mark = [&](Node &node, State state) {
        node.state = state;
        for (Node *child : node.children) {
            mark(*child, state);
        }
    };

    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.next != nullptr) {
            if (node.state != State::Unchanged) {
                mark(*node.next, node.state);
            }
            if (node.moved) {
                markTreeAsMoved(node.next);
            }
            return visit(*node.next);
        }

        for (Node *child : node.children) {
            visit(*child);
        }
    };
    visit(*getRoot());
}

boost::string_ref
Tree::intern(std::string &&str)
{
    internPool.push_back(std::move(str));
    return internPool.back();
}
