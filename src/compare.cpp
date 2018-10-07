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

#include "compare.hpp"

#include <cassert>

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/optional.hpp>
#include <dtl/dtl.hpp>

#include "utils/strings.hpp"
#include "utils/time.hpp"
#include "Language.hpp"
#include "change-distilling.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"

namespace {

// Coordinates tree comparison.
class Comparator
{
public:
    // Records arguments for future use.
    Comparator(Tree &T1, Tree &T2, TimeReport &tr, bool coarse,
               bool skipRefine);

public:
    // Launches comparison.
    void compare();

private:
    // Performs comparison of trees available at this level and if necessary of
    // trees from the following levels.
    void compare(Node *T1, Node *T2);
    // Recursively compares nodes that are marked as changed.
    void compareChanged(Node *node);
    // Flattens two trees simultaneously.
    void flatten(Node *x, Node *y);
    // Attempts to flatten subtrees on a specific level.  Returns `true` if
    // anything was flattened.
    bool flatten(Node *x, Node *y, int level);
    // Either attempts to flatten nodes of a subtree on a specific level or
    // simulates it to see how many nodes would be flattened.  Returns number of
    // nodes flattened or that would be flattened.
    int flatten(Node *n, int level, bool dry);
    // Detects moves within subtree.
    void detectMoves(Node *x);
    // Finds first movable parent.  Stops at the root (no parent).  Might return
    // `nullptr`.
    const Node * getParent(const Node *x);
    // Performs single-level move detection for nodes with fixed structure.
    void detectMovesInFixedStructure(Node *x, Node *y);
    // Computes position of auxiliary node of a parent with fixed structure
    // offset by added and removed nodes.
    int getMovePosOfAux(Node *node);
    // Marks subtree of this node and its relative accounting for special cases.
    void markMoved(Node *x);
    // This is a workaround to compensate the fact that travelling nodes (called
    // postponed on parser/lexer level) can fall off container when they are in
    // front of it.
    bool isTravellingPair(const Node *x, const Node *y);

private:
    Tree &T1, &T2;       // Two trees being compared.
    Language &lang;      // Language being used.
    TimeReport &tr;      // Time keeper.
    bool coarse;         // Do only fine-grained comparison.
    bool skipRefine;     // Do not perform fine-grained refining.
    Distiller distiller; // Implementation of change-distilling algorithm.
};

template <typename T, typename... Args>
auto bind(const T &f, Args&&... a)
    -> decltype(std::bind(f, std::forward<Args>(a)..., std::placeholders::_1))
{
    return std::bind(f, std::forward<Args>(a)..., std::placeholders::_1);
}

}

static void setParentLinks(Node *x, Node *parent);
static void refine(Node &node);

Comparator::Comparator(Tree &T1, Tree &T2, TimeReport &tr, bool coarse,
                       bool skipRefine)
    : T1(T1), T2(T2), lang(*T1.getLanguage()),
      tr(tr), coarse(coarse), skipRefine(skipRefine), distiller(lang)
{
    // XXX: the assumption is that both trees have the same language.
    //      Might be a good idea to actually check this somewhere.
}

void
Comparator::compare()
{
    compare(T1.getRoot(), T2.getRoot());
}

void
Comparator::compare(Node *T1, Node *T2)
{
    struct Match
    {
        Node *x;
        Node *y;
        float similarity;
        bool identical;
    };

    auto diffingTimer = tr.measure("diffing");

    tr.measure("coarse-reduction"), reduceTreesCoarse(T1, T2);

    if (!coarse) {
        tr.measure("diffing"), ted(*T1, *T2);
        return;
    }

    std::vector<Match> matches;

    auto timer = tr.measure("distilling");
    for (Node *t1Child : T1->children) {
        if (t1Child->satellite) {
            continue;
        }
        boost::optional<std::string> subtree1;
        const std::string st1 = printSubTree(*t1Child, false);
        DiceString subtree1Dice(st1);
        for (Node *t2Child : T2->children) {
            if (t2Child->satellite) {
                continue;
            }

            // XXX: here mismatched labels are included in similarity
            //      measurement, which affects it negatively
            const std::string st2 = printSubTree(*t2Child, false);
            DiceString subtree2Dice(st2);
            const float similarity = subtree1Dice.compare(subtree2Dice);
            bool identical = (similarity == 1.0f);
            if (identical) {
                if (!subtree1) {
                    subtree1 = printSubTree(*t1Child, true);
                }
                identical = (subtree1 == printSubTree(*t2Child, true));
            }
            if ((t1Child->label == t2Child->label && similarity >= 0.6f) ||
                (t1Child->label != t2Child->label && similarity >= 0.8f)) {
                matches.push_back({ t1Child, t2Child, similarity, identical });
            }
        }
    }

    std::stable_sort(matches.begin(), matches.end(),
                     [](const Match &a, const Match &b) {
                         if (a.identical || b.identical) {
                             return b.identical < a.identical;
                         }
                         return b.similarity < a.similarity;
                     });

    for (const Match &match : matches) {
        if (match.x->relative != nullptr || match.y->relative != nullptr) {
            continue;
        }

        Node *subT1 = match.x, *subT2 = match.y;
        distiller.distill(*subT1, *subT2);

        if (subT1->relative == subT2 && subT1->next && subT2->next &&
            !subT1->next->last && !subT2->next->last) {
            // Process next layers of nodes which were identified as updated the
            // same way compareChanged() does it.
            compare(subT1->next, subT2->next);
            subT1->state = State::Unchanged;
            subT2->state = State::Unchanged;
            // Mark the trees as satellites to exclude them from distilling.
            subT1->satellite = true;
            subT2->satellite = true;
        }
    }

    // Flatten unmatched trees into parent tree of their roots before doing
    // common distilling.
    flatten(T1, T2);

    distiller.distill(*T1, *T2);
    setParentLinks(T1, nullptr);
    setParentLinks(T2, nullptr);
    detectMoves(T1);

    timer.measure("descending");
    compareChanged(T1);

    if (!skipRefine) {
        refine(*T1);
    }
}

void
Comparator::compareChanged(Node *node)
{
    for (Node *x : node->children) {
        Node *y = x->relative;
        if (y != nullptr && x->next != nullptr && y->next != nullptr) {
            if (!x->next->last && !x->satellite) {
                x->state = State::Unchanged;
                y->state = State::Unchanged;
                compare(x->next, y->next);
            }
        } else {
            compareChanged(x);
        }
    }
}

void
Comparator::flatten(Node *x, Node *y)
{
    int flattenLevel = 0;
    if (flatten(x, y, flattenLevel)) {
        ++flattenLevel;
        flatten(x, y, flattenLevel);
    }

    while (++flattenLevel < 4) {
        if (flatten(x, y, flattenLevel)) {
            break;
        }
    }
}

bool
Comparator::flatten(Node *x, Node *y, int level)
{
    const int wouldFlatten = flatten(x, level, true) + flatten(y, level, true);
    // XXX: hard-coded threshold
    if (wouldFlatten > 0 && wouldFlatten < 5) {
        return (flatten(x, level, false) + flatten(y, level, false) > 0);
    }
    return false;
}

int
Comparator::flatten(Node *n, int level, bool dry)
{
    int flattened = 0;

    if (n->satellite) {
        return 0;
    }

    for (Node *&c : n->children) {
        if (c->satellite || (c->next != nullptr && c->next->last)) {
            continue;
        }

        if (c->next == nullptr && !c->children.empty()) {
            flattened += flatten(c, level, dry);
            continue;
        }

        if (c->relative != nullptr) {
            continue;
        }

        if (c->next != nullptr && lang.canBeFlattened(n, c, level)) {
            if (!dry) {
                c = c->next;
            }
            ++flattened;
        }
    }

    return flattened;
}

static void
setParentLinks(Node *x, Node *parent)
{
    x->parent = parent;
    for (Node *child : x->children) {
        setParentLinks(child, x);
    }
}

void
Comparator::detectMoves(Node *x)
{
    Node *const y = x->relative;
    const Node *const px = getParent(x);
    const Node *const py = (y ? getParent(y) : nullptr);

    // Mark nodes which switched their parents as moved.
    if (px && py && px->relative != py && !lang.isUnmovable(x)) {
        markMoved(x);
    }

    if (x->children.empty()) {
        return;
    }

    if (y != nullptr && lang.hasMoveableItems(x)) {
        if (lang.hasFixedStructure(x)) {
            detectMovesInFixedStructure(x, y);
            for (Node *child : x->children) {
                detectMoves(child);
            }
            return;
        }

        auto cmp = [](Node *x, Node *y) {
            return x->relative == y;
        };

        dtl::Diff<Node *, cpp17::pmr::vector<Node *>, decltype(cmp)>
            diff(x->children, y->children, cmp);
        diff.compose();

        for (const auto &d : diff.getSes().getSequence()) {
            if (d.second.type == dtl::SES_DELETE) {
                markMoved(x->children[d.second.beforeIdx - 1]);
            }
        }
    }

    for (Node *child : x->children) {
        detectMoves(child);
    }
}

const Node *
Comparator::getParent(const Node *x)
{
    do {
        x = x->parent;
    } while (x != nullptr && lang.isUnmovable(x));
    return x;
}

void
Comparator::detectMovesInFixedStructure(Node *x, Node *y)
{
    std::vector<Node *> xChildren, yChildren;

    // Extract payload of the structure to compare it separately.
    auto predicate = bind(&Language::isPayloadOfFixed, &lang);
    xChildren.reserve(x->children.size());
    std::copy_if(x->children.cbegin(), x->children.cend(),
                 std::back_inserter(xChildren), predicate );
    yChildren.reserve(y->children.size());
    std::copy_if(y->children.cbegin(), y->children.cend(),
                 std::back_inserter(yChildren), predicate );

    assert(xChildren.size() == yChildren.size() && "Must be in sync.");

    // Because number of children is fixed, checking for matching positions
    // suffices.
    for (unsigned int i = 0U; i < xChildren.size(); ++i) {
        Node *const c = xChildren[i];
        auto it = std::find(yChildren.cbegin(), yChildren.cend(), c->relative);
        if (it - yChildren.cbegin() != i) {
            markMoved(c);
        }
    }

    // Move detection for auxiliary nodes need to ignore payload nodes and
    // account for addition/deletion properly.
    for (Node *c : x->children) {
        Node *const r = c->relative;
        if (r != nullptr && getMovePosOfAux(c) != getMovePosOfAux(r)) {
            markMoved(c);
        }
    }
}

int
Comparator::getMovePosOfAux(Node *node)
{
    cpp17::pmr::vector<Node *> &children = node->parent->children;
    int pos = 0;
    for (Node *child : children) {
        if (child == node) {
            break;
        }
        if (child->relative != nullptr && !lang.isPayloadOfFixed(child) &&
            child->relative->parent == node->relative->parent &&
            !child->moved) {
            ++pos;
        }
    }
    return pos;
}

void
Comparator::markMoved(Node *x)
{
    Node *const y = x->relative;
    if (y != nullptr && !isTravellingPair(x, y) && !isTravellingPair(y, x)) {
        T1.markTreeAsMoved(x);
        T2.markTreeAsMoved(y);
    }
}

bool
Comparator::isTravellingPair(const Node *x, const Node *y)
{
    if (!lang.isTravellingNode(x)) {
        return false;
    }

    // Go up until we find parent that has sibling after previously visited
    // node.
    while (x->parent != nullptr) {
        auto it = std::find(x->parent->children.cbegin(),
                            x->parent->children.cend(),
                            x);

        while (++it != x->parent->children.cend() &&
               lang.isTravellingNode(*it)) {
            // Skip other traveling nodes.
        }
        if (it != x->parent->children.cend()) {
            x = *it;
            break;
        }

        x = x->parent;
    }

    if (x->parent == nullptr) {
        return false;
    }

    // Go down into left-most node trying to find node which is relative to y.
    while (!x->children.empty()) {
        if (x->relative == y->parent) {
            return true;
        }

        x = x->children.front();
    }

    return false;
}

static void
refine(Node &node)
{
    if (node.satellite) {
        return;
    }

    if (node.leaf && node.state == State::Updated && node.next != nullptr) {
        node.state = State::Unchanged;
        node.relative->state = State::Unchanged;

        Node *subT1 = node.next, *subT2 = node.relative->next;
        ted(*subT1, *subT2);
    }

    for (Node *child : node.children) {
        refine(*child);
    }
}

void
compare(Tree &T1, Tree &T2, TimeReport &tr, bool coarse, bool skipRefine)
{
    return Comparator(T1, T2, tr, coarse, skipRefine).compare();
}
