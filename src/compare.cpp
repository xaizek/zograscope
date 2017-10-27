#include "compare.hpp"

#include <algorithm>
#include <vector>

#include <boost/optional.hpp>
#include <dtl/dtl.hpp>

#include "change-distilling.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static bool flatten(Node *n, int level);
static void markParents(Node *x, Node *parent);
static void detectMoves(Node *x);
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

            // XXX: here mismatched lables are included in similarity
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
        match.x->satellite = match.identical;
        match.y->satellite = match.identical;
        match.x->relative = match.y;
        match.y->relative = match.x;
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
    markParents(T1, nullptr);
    markParents(T2, nullptr);
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
markParents(Node *x, Node *parent)
{
    x->parent = parent;
    if (!isUnmovable(x)) {
        parent = x;
    }
    for (Node *child : x->children) {
        markParents(child, parent);
    }
}

static void
detectMoves(Node *x)
{
    if (x->parent && x->relative && x->relative->parent &&
        x->parent && x->relative && x->relative->parent &&
        x->parent->relative != x->relative->parent &&
        !isUnmovable(x)) {
        // Mark nodes which switched their parents as moved.
        markTreeAsMoved(x);
        markTreeAsMoved(x->relative);
    }

    if (x->children.empty()) {
        return;
    }

    auto cmp = [](Node *x, Node *y) {
        return x->relative == y;
    };

    if (x->relative != nullptr && hasMoveableItems(x)) {
        dtl::Diff<Node *, std::vector<Node *>, decltype(cmp)>
            diff(x->children, x->relative->children, cmp);
        diff.compose();

        for (const auto &d : diff.getSes().getSequence()) {
            if (d.second.type == dtl::SES_DELETE) {
                Node *node = x->children[d.second.beforeIdx - 1];
                if (node->relative != nullptr) {
                    markTreeAsMoved(node);
                    markTreeAsMoved(node->relative);
                }
            }
        }
    }

    for (Node *child : x->children) {
        detectMoves(child);
    }
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
