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
#include <unordered_map>
#include <utility>
#include <vector>

#include "utils/strings.hpp"
#include "utils/trees.hpp"
#include "Language.hpp"
#include "STree.hpp"
#include "TreeBuilder.hpp"
#include "decoration.hpp"
#include "types.hpp"

// How many neighbours to consider on each side when computing overlap.
constexpr int subtreeOverlapSize = 3;

static void putNodeChild(Node &parent, Node *child, const Language *lang);
static void preStringifyPTree(const std::string &contents,
                              PNode *node, const Language *lang, int tabWidth,
                              cpp17::pmr::vector<char> &stringified);
static boost::string_ref
stringifyPNode(const cpp17::pmr::vector<char> &stringified, const PNode *node);
static void preStringifyPNode(const std::string &contents, PNode *node,
                              const Language *lang, int tabWidth,
                              cpp17::pmr::vector<char> &stringified);
static std::string stringifyPNodeSpelling(const std::string &contents,
                                          const PNode *node, int tabWidth);
static int maxStringifiedSize(boost::string_ref contents, int tabWidth);
static void postOrder(Node &node, std::vector<Node *> &v);
static std::unordered_map<std::size_t, std::vector<int>>
hashChildren(Node &node);
static std::size_t hashNode(const Node *node);
static void matchTrees(Node *x, Node *y);
static int rateChildOverlap(int xi, const cpp17::pmr::vector<Node *> &c1,
                            int yi, const cpp17::pmr::vector<Node *> &c2);
static void markAsMoved(Node *node, Language &lang);
static void dumpTree(std::ostream &os, const Node *node, const Language *lang,
                     std::vector<bool> &trace, int depth);
static void dumpNode(std::ostream &os, const Node *node, const Language *lang);

Tree::Tree(std::unique_ptr<Language> lang, int tabWidth,
           const std::string &contents, const PNode *node, allocator_type al)
    : lang(std::move(lang)), nodes(al), stringified(al), internPool(al),
      tabWidth(tabWidth)
{
    stringified.reserve(maxStringifiedSize(contents, tabWidth));
    const char *buf = stringified.data();

    preStringifyPTree(contents, const_cast<PNode *>(node), this->lang.get(),
                      tabWidth, stringified);
    root = materializePNode(contents, node);

    assert(stringified.data() == buf && "Stringified buffer got relocated!");
    (void)buf;
}

Tree::Tree(std::unique_ptr<Language> lang, int tabWidth,
           const std::string &contents, const SNode *node, allocator_type al)
    : lang(std::move(lang)), nodes(al), stringified(al), internPool(al),
      tabWidth(tabWidth)
{
    stringified.reserve(maxStringifiedSize(contents, tabWidth));
    const char *buf = stringified.data();

    preStringifyPTree(contents, node->value, this->lang.get(), tabWidth,
                      stringified);
    root = materializeSNode(contents, node, nullptr);

    assert(stringified.data() == buf && "Stringified buffer got relocated!");
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
                  const Language *lang, int tabWidth,
                  cpp17::pmr::vector<char> &stringified)
{
    struct {
        const std::string &contents;
        const Language *lang;
        int tabWidth;
        cpp17::pmr::vector<char> &out;
        void run(PNode *node)
        {
            node->value.postponedFrom = out.size();

            if (node->line != 0 && node->col != 0) {
                preStringifyPNode(contents, node, lang, tabWidth, out);
            }

            for (PNode *child : node->children) {
                run(child);
            }

            if (node->line == 0 || node->col == 0) {
                node->value.postponedTo = out.size()
                                        - node->value.postponedFrom;
            }
        }
    } visitor { contents, lang, tabWidth, stringified };

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
        n.spelling = intern(stringifyPNodeSpelling(contents, node, tabWidth));
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
stringifyPNode(const cpp17::pmr::vector<char> &stringified, const PNode *node)
{
    return boost::string_ref(stringified.data() + node->value.postponedFrom,
                             node->value.postponedTo);
}

// Turns node into a string.  Stores boundaries of this node's label in
// value.postponedFrom (start index) and value.postponedTo (length).
static void
preStringifyPNode(const std::string &contents, PNode *node,
                  const Language *lang, int tabWidth,
                  cpp17::pmr::vector<char> &stringified)
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
                stringified.push_back('\n');
                leadingWhitespace = lang->shouldDropLeadingWS(node->stype);
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                col += width;
                if (!leadingWhitespace) {
                    stringified.insert(stringified.cend(), width, ' ');
                }
                break;
            case ' ':
                ++col;
                if (!leadingWhitespace) {
                    stringified.push_back(' ');
                }
                break;

            default:
                ++col;
                stringified.push_back(c);
                leadingWhitespace = false;
                break;
        }
    }

    node->value.postponedTo = stringified.size() - node->value.postponedFrom;
}

// Computes node label only expanding tabs in it.
static std::string
stringifyPNodeSpelling(const std::string &contents, const PNode *node,
                       int tabWidth)
{
    boost::string_ref sr(contents.c_str() + node->value.from, node->value.len);

    std::string str;
    str.reserve(maxStringifiedSize(sr, tabWidth));

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
maxStringifiedSize(boost::string_ref contents, int tabWidth)
{
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
    std::unordered_map<std::size_t, std::vector<int>> hashed1 =
        hashChildren(*T1);
    std::unordered_map<std::size_t, std::vector<int>> hashed2 =
        hashChildren(*T2);

    struct Pair {
        Pair(std::vector<int> *from, std::vector<int> *to) :
            from(from), to(to)
        { }

        std::vector<int> *from;
        std::vector<int> *to;
    };

    std::vector<Pair> pairs;
    pairs.reserve(std::min(hashed1.size(), hashed2.size()));
    for (auto &entry : hashed1) {
        auto it = hashed2.find(entry.first);
        if (it != hashed2.cend()) {
            pairs.emplace_back(&entry.second, &it->second);
        }
    }

    // We want to match nodes from less frequent (no ambiguity for 1-1) to most
    // frequent.  Stable sorting is for reproducible results.
    std::stable_sort(pairs.begin(), pairs.end(),
                     [](const Pair &a, const Pair &b) {
                         auto n = a.from->size(), m = b.from->size();
                         return (n < m)
                             || (n == m && a.to->size() < b.to->size());
                     });

    for (Pair &pair : pairs) {
        int n = pair.from->size();
        int m = pair.to->size();

        // Having larger set as "to" allows to explore more options.
        bool swap = (n > m);
        if (swap) {
            std::swap(pair.from, pair.to);
            std::swap(n, m);
        }
        const cpp17::pmr::vector<Node *> &ci = (swap ? T2 : T1)->children;
        const cpp17::pmr::vector<Node *> &cj = (swap ? T1 : T2)->children;

        // Match here is obvious, skip computing the overlap.
        if (n == 1 && m == 1) {
            Node *x = ci[pair.from->front()];
            Node *y = cj[pair.to->front()];

            matchTrees(x, y);
            x->satellite = true;
            y->satellite = true;
            continue;
        }

        for (int i : *pair.from) {
            int bestI = -1;
            int bestJ = -1;
            int bestOverlap = -1;

            for (int j : *pair.to) {
                if (cj[j]->satellite) {
                    continue;
                }

                int overlap = rateChildOverlap(i, ci, j, cj);
                if (overlap > bestOverlap) {
                    bestOverlap = overlap;
                    bestI = i;
                    bestJ = j;
                }
            }

            // There must always be a match because of `n <= m` precondition.
            matchTrees(ci[bestI], cj[bestJ]);
            ci[bestI]->satellite = true;
            cj[bestJ]->satellite = true;
        }
    }
}

// Hashes direct non-satellite children of the node individually.
static std::unordered_map<std::size_t, std::vector<int>>
hashChildren(Node &node)
{
    std::unordered_map<std::size_t, std::vector<int>> hashes;
    for (int i = 0, n = node.children.size(); i < n; ++i) {
        Node *child = node.children[i];
        if (!child->satellite) {
            hashes[hashNode(child)].push_back(i);
        }
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

// Matches corresponding nodes of two trees.  Assumption is that matched nodes
// have exactly the same structure.
static void
matchTrees(Node *x, Node *y)
{
    x->state = State::Unchanged;
    y->state = State::Unchanged;
    x->relative = y;
    y->relative = x;

    for (auto l = x->children.begin(), r = y->children.begin();
         l != x->children.end() && r != y->children.end();
         ++l, ++r) {
        matchTrees(*l, *r);
    }

    if (x->next && !x->next->last && y->next && !y->next->last) {
        matchTrees(x->next, y->next);
    }
}

// Computes rate that depends on number and position of neighbouring nodes of
// `c1[xi]` that match corresponding (by offset) nodes of `c2[yi]`.  This
// heuristics glues unmatched nodes to their already matched neighbours and
// resolves ties quite well.  Holes at the ends (too far left or right) of both
// arguments are considered a match to match border nodes to border nodes.
static int
rateChildOverlap(int xi, const cpp17::pmr::vector<Node *> &c1,
                 int yi, const cpp17::pmr::vector<Node *> &c2)
{
    // TODO: maybe try matching true satellitels (separators) with each other by
    //       value

    int overlap = 0;

    int maxLeftOffset = std::min({ xi, yi, subtreeOverlapSize });
    for (int i = 1; i <= maxLeftOffset; ++i) {
        overlap += (c1[xi - i]->relative == c2[yi - i]);
    }
    for (int i = maxLeftOffset + 1; i <= subtreeOverlapSize; ++i) {
        if (xi - i < 0 && yi - i < 0) {
            overlap += subtreeOverlapSize - i + 1;
        }
    }

    int n = c1.size();
    int m = c2.size();
    int maxRightOffset = std::min({ n - 1 - xi,
                                    m - 1 - yi,
                                    subtreeOverlapSize });
    for (int i = 1; i <= maxRightOffset; ++i) {
        overlap += (c1[xi + i]->relative == c2[yi + i]);
    }
    for (int i = maxRightOffset + 1; i <= subtreeOverlapSize; ++i) {
        if (xi + i >= n && yi + i >= m) {
            overlap += subtreeOverlapSize - i + 1;
        }
    }

    return overlap;
}

std::string
printSubTree(const Node &root, bool withComments, int size_hint)
{
    // Reminder: making a separate version for use with custom allocator reduces
    //           number of allocations, but doesn't impact neither performance
    //           nor parallelism.

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
Tree::markInPreOrder()
{
    struct {
        int id = 1;
        void markInPreOrder(Node &node) {
            node.poID = id++;
            if (node.next != nullptr) {
                if (!node.next->leaf) {
                    markInPreOrder(*node.next);
                }
            } else {
                for (Node *child : node.children) {
                    markInPreOrder(*child);
                }
            }
        }
    } visitor;

    if (root != nullptr) {
        visitor.markInPreOrder(*root);
    }
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
