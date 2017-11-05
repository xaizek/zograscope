#include "compare.hpp"

#include <cassert>

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/optional.hpp>
#include <dtl/dtl.hpp>

#include "change-distilling.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static bool flatten(Node *n, int level);
static void setParentLinks(Node *x, Node *parent);
static void detectMoves(Node *x);
static const Node * getParent(const Node *x);
static void detectMovesInFixedStructure(Node *x, Node *y);
static int getMovePosOfAux(Node *node);
static void markMoved(Node *x);
static bool isTravellingPair(const Node *x, const Node *y);
static void refine(Node &node);

void
compare(Node *T1, Node *T2, TimeReport &tr, bool coarse, bool skipRefine)
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

    auto refine = [skipRefine](Node &root) {
        if (!skipRefine) {
            ::refine(root);
        }
    };

    std::vector<Match> matches;

    auto timer = tr.measure("distilling");
    for (Node *t1Child : T1->children) {
        if (t1Child->satellite) {
            continue;
        }
        boost::optional<std::string> subtree1;
        DiceString subtree1Dice = printSubTree(*t1Child, false);
        for (Node *t2Child : T2->children) {
            if (t2Child->satellite) {
                continue;
            }

            // XXX: here mismatched labels are included in similarity
            //      measurement, which affects it negatively
            DiceString subtree2Dice = printSubTree(*t2Child, false);
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
        distill(*subT1, *subT2);
    }

    int flattenLevel = 0;
    if (flatten(T1, flattenLevel) | flatten(T2, flattenLevel)) {
        ++flattenLevel;
        flatten(T1, flattenLevel);
        flatten(T2, flattenLevel);
    }

    // Flatten unmatched trees into parent tree of their roots before doing
    // common distilling.
    while (++flattenLevel < 3) {
        if (flatten(T1, flattenLevel) | flatten(T2, flattenLevel)) {
            break;
        }
    }
    distill(*T1, *T2);
    setParentLinks(T1, nullptr);
    setParentLinks(T2, nullptr);
    detectMoves(T1);

    timer.measure("recursing");

    std::function<void(Node *)> visit = [&](Node *node) {
        for (Node *x : node->children) {
            if (x->relative != nullptr && x->next != nullptr &&
                x->relative->next != nullptr) {
                if (!x->next->last && !x->satellite) {
                    x->state = State::Unchanged;
                    x->relative->state = State::Unchanged;
                    compare(x->next, x->relative->next, tr, coarse, skipRefine);
                }
            } else {
                visit(x);
            }
        }
    };

    visit(T1);
    refine(*T1);
}

static bool
flatten(Node *n, int level)
{
    bool flattened = false;

    for (Node *&c : n->children) {
        if (c->next != nullptr && c->next->last) {
            continue;
        }

        if (c->next == nullptr && !c->children.empty()) {
            flattened |= flatten(c, level);
            continue;
        }

        if (c->relative != nullptr) {
            continue;
        }

        if (c->next != nullptr && canBeFlattened(n, c, level)) {
            c = c->next;
            flattened = true;
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

static void
detectMoves(Node *x)
{
    Node *const y = x->relative;
    const Node *const px = getParent(x);
    const Node *const py = (y ? getParent(y) : nullptr);

    // Mark nodes which switched their parents as moved.
    if (px && py && px->relative != py && !isUnmovable(x)) {
        markMoved(x);
    }

    if (x->children.empty()) {
        return;
    }

    auto cmp = [](Node *x, Node *y) {
        return x->relative == y;
    };

    if (y != nullptr && hasMoveableItems(x)) {
        if (hasFixedStructure(x)) {
            detectMovesInFixedStructure(x, y);
            for (Node *child : x->children) {
                detectMoves(child);
            }
            return;
        }

        dtl::Diff<Node *, std::vector<Node *>, decltype(cmp)> diff(x->children,
                                                                   y->children,
                                                                   cmp);
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

static const Node *
getParent(const Node *x)
{
    do {
        x = x->parent;
    } while (x != nullptr && isUnmovable(x));
    return x;
}

// Performs single-level move detection for nodes with fixed structure.
static void
detectMovesInFixedStructure(Node *x, Node *y)
{
    std::vector<Node *> xChildren, yChildren;

    // Extract payload of the structure to compare it separately.
    xChildren.reserve(x->children.size());
    std::copy_if(x->children.cbegin(), x->children.cend(),
                 std::back_inserter(xChildren), &isPayloadOfFixed);
    yChildren.reserve(y->children.size());
    std::copy_if(y->children.cbegin(), y->children.cend(),
                 std::back_inserter(yChildren), &isPayloadOfFixed);

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

// Computes position of auxiliary node of a parent with fixed structure offset
// by added and removed nodes.
static int
getMovePosOfAux(Node *node)
{
    std::vector<Node *> &children = node->parent->children;
    int pos = 0;
    for (Node *child : children) {
        if (child == node) {
            break;
        }
        if (child->relative != nullptr && !isPayloadOfFixed(child) &&
            child->relative->parent == node->relative->parent &&
            !child->moved) {
            ++pos;
        }
    }
    return pos;
}

// Marks subtree of this node and its relative accounting for special cases.
static void
markMoved(Node *x)
{
    Node *const y = x->relative;
    if (y != nullptr && !isTravellingPair(x, y) && !isTravellingPair(y, x)) {
        markTreeAsMoved(x);
        markTreeAsMoved(y);
    }
}

// This is a workaround to compensate the fact that travelling nodes (called
// postponed on parser/lexer level) can fall off container when they are in
// front of it.
static bool
isTravellingPair(const Node *x, const Node *y)
{
    if (!isTravellingNode(x)) {
        return false;
    }

    // Go up until we find parent that has sibling after previously visited
    // node.
    while (x->parent != nullptr) {
        auto it = std::find(x->parent->children.cbegin(),
                            x->parent->children.cend(),
                            x);

        while (++it != x->parent->children.cend() && isTravellingNode(*it)) {
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

    if (node.line != 0 && node.col != 0 && node.state == State::Updated) {
        node.state = State::Unchanged;
        node.relative->state = State::Unchanged;

        Node *subT1 = node.next, *subT2 = node.relative->next;
        ted(*subT1, *subT2);
    }

    for (Node *child : node.children) {
        refine(*child);
    }
}
