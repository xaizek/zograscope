// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
#include "stypes.hpp"
#include "types.hpp"

static Node * materializeSNode(Tree &tree, const std::string &contents,
                               const SNode *node);
static const PNode * leftmostChild(const PNode *node);
static std::string materializePTree(const std::string &contents,
                                    const PNode *node);
static Node * materializePNode(Tree &tree, const std::string &contents,
                               const PNode *node);
static void materializeLabel(const std::string &contents, const PNode *node,
                             bool spelling, std::string &str);
static void postOrder(Node &node, std::vector<Node *> &v);
static std::vector<std::size_t> hashChildren(const Node &node);
static std::size_t hashNode(const Node *node);
static void markAsMoved(Node *node, Language &lang);
static void dumpTree(std::ostream &os, const Node *node,
                     std::vector<bool> &trace, int depth);
static void dumpNode(std::ostream &os, const Node *node);

Tree::Tree(std::unique_ptr<Language> lang, const std::string &contents,
           const PNode *node, allocator_type al)
    : lang(std::move(lang)), nodes(al)
{
    root = materializePNode(*this, contents, node);
}

Tree::Tree(std::unique_ptr<Language> lang, const std::string &contents,
           const SNode *node, allocator_type al)
    : lang(std::move(lang)), nodes(al)
{
    root = materializeSNode(*this, contents, node);
}

static Node *
materializeSNode(Tree &tree, const std::string &contents, const SNode *node)
{
    Node &n = tree.makeNode();
    n.stype = node->value->stype;
    n.satellite = (n.stype == SType::Separator);

    if (node->children.empty()) {
        const PNode *leftmostLeaf = leftmostChild(node->value);

        n.label = materializePTree(contents, node->value);
        n.line = leftmostLeaf->line;
        n.col = leftmostLeaf->col;
        n.next = materializePNode(tree, contents, node->value);
        n.next->last = true;
        n.type = n.next->type;
        n.leaf = (n.line != 0 && n.col != 0);
        return &n;
    }

    const Language *const lang = tree.getLanguage();
    n.children.reserve(node->children.size());
    for (SNode *child : node->children) {
        Node *newChild = materializeSNode(tree, contents, child);

        if (lang->shouldSplice(node->value->stype, newChild)) {
            if (newChild->next != nullptr) {
                // Make sure we don't splice last layer.
                if (newChild->next->last) {
                    n.children.emplace_back(newChild);
                    continue;
                }

                newChild = newChild->next;
            }

            n.children.insert(n.children.cend(),
                              newChild->children.cbegin(),
                              newChild->children.cend());
        } else {
            n.children.emplace_back(newChild);
        }
    }

    auto valueChild = std::find_if(node->children.begin(), node->children.end(),
                                   [lang](const SNode *node) {
                                       const SType stype = node->value->stype;
                                       return lang->isValueNode(stype);
                                   });
    if (valueChild != node->children.end()) {
        n.label = materializePTree(contents, (*valueChild)->value);
        n.valueChild = valueChild - node->children.begin();
    } else {
        n.valueChild = -1;
    }

    // Move certain nodes onto the next layer.
    if (lang->isLayerBreak(n.stype)) {
        Node &nextLevel = tree.makeNode();
        nextLevel.next = &n;
        nextLevel.stype = n.stype;
        nextLevel.label = n.label.empty() ? printSubTree(n, false) : n.label;
        return &nextLevel;
    }

    return &n;
}

// Finds the leftmost child of the node.
static const PNode *
leftmostChild(const PNode *node)
{
    return node->children.empty()
         ? node
         : leftmostChild(node->children.front());
}

static std::string
materializePTree(const std::string &contents, const PNode *node)
{
    struct {
        const std::string &contents;
        std::string out;
        void run(const PNode *node)
        {
            if (node->line != 0 && node->col != 0) {
                materializeLabel(contents, node, false, out);
            }

            for (const PNode *child : node->children) {
                run(child);
            }
        }
    } visitor { contents, {} };

    visitor.run(node);

    return visitor.out;
}

static Node *
materializePNode(Tree &tree, const std::string &contents, const PNode *node)
{
    const Type type = tree.getLanguage()->mapToken(node->value.token);

    if (type == Type::Virtual && node->children.size() == 1U) {
        return materializePNode(tree, contents, node->children[0]);
    }

    Node &n = tree.makeNode();
    materializeLabel(contents, node, false, n.label);
    if (node->stype == SType::Comment) {
        materializeLabel(contents, node, true, n.spelling);
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
        n.children.push_back(materializePNode(tree, contents, child));
    }

    return &n;
}

static void
materializeLabel(const std::string &contents, const PNode *node, bool spelling,
                 std::string &str)
{
    // XXX: lexer also has such variable and they need to be synchronized
    //      (actually we should pass this to lexer).
    enum { tabWidth = 4 };

    str.reserve(str.size() + node->value.len);

    bool leadingWhitespace = false;
    int col = node->col;
    for (std::size_t i = node->value.from;
         i < node->value.from + node->value.len; ++i) {
        switch (const char c = contents[i]) {
            int width;

            case '\n':
                col = 1;
                str += '\n';
                leadingWhitespace = !spelling
                                 && node->stype == SType::Comment;
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                col += width;
                if (!leadingWhitespace) {
                    str.append(width, ' ');
                }
                break;
            case ' ':
                ++col;
                if (!leadingWhitespace) {
                    str += ' ';
                }
                break;

            default:
                ++col;
                str += c;
                leadingWhitespace = false;
                break;
        }
    }
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
                T1->children[i]->relative = T2->children[j];
                T1->children[i]->satellite = true;
                T2->children[j]->relative = T1->children[i];
                T2->children[j]->satellite = true;
                break;
            }
        }
    }
}

static std::vector<std::size_t>
hashChildren(const Node &node)
{
    if (node.next != nullptr) {
        return hashChildren(*node.next);
    }

    std::vector<std::size_t> hashes;
    hashes.reserve(node.children.size());
    for (const Node *child : node.children) {
        hashes.push_back(hashNode(child));
    }
    return hashes;
}

static std::size_t
hashNode(const Node *node)
{
    if (node->next != nullptr) {
        return hashNode(node->next);
    }

    std::size_t hash = std::hash<std::string>()(node->label);
    for (const Node *child : node->children) {
        boost::hash_combine(hash, hashNode(child));
    }
    return hash;
}

std::string
printSubTree(const Node &root, bool withComments)
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
                out += node.label;
            }

            for (const Node *child : node.children) {
                run(*child);
            }
        }
    } visitor { withComments, {} };

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
        std::vector<bool> trace;
        dumpTree(std::cout, root, trace, 0);
    }
}

// Dumps subtree onto standard output.
static void
dumpTree(std::ostream &os, const Node *node, std::vector<bool> &trace,
         int depth)
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
    dumpNode(os, node);

    trace.push_back(false);
    for (unsigned int i = 0U, n = node->children.size(); i < n; ++i) {
        Node *child = node->children[i];

        trace.back() = (i == n - 1U);
        dumpTree(os, child, trace, depth);

        if (child->next != nullptr && !child->next->last) {
            trace.push_back(true);
            dumpTree(os, child->next, trace, depth + 1);
            trace.pop_back();
        }
    }
    trace.pop_back();
}

// Dumps single node into a stream.
static void
dumpNode(std::ostream &os, const Node *node)
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

    auto l = [](const std::string &s) {
        return '`' + boost::replace_all_copy(s, "\n", "<NL>") + '`';
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

    os << (labelHi << l(node->label))
       << (idHi << " #" << node->poID);

    os << (node->satellite ? ", Satellite" : "") << ", "
       << (typeHi << "Type::" << node->type) << ", "
       << (stypeHi << "SType::" << node->stype);

    if (node->relative != nullptr) {
        os << (relHi << " -> ") << (relLabelHi << l(node->relative->label))
           << (idHi << " #" << node->relative->poID);
    }

    os << '\n';
}
