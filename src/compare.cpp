#include "compare.hpp"

#include <algorithm>
#include <vector>

#include "change-distilling.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

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
        std::string subtree1 = printSubTree(*t1Child);
        for (Node *t2Child : T2->children) {
            if (t2Child->satellite) {
                continue;
            }

            // XXX: here mismatched lables are included in similarity
            //      measurement, which affects it negatively
            std::string subtree2 = printSubTree(*t2Child);
            const bool identical = (subtree1 == subtree2);
            const float similarity = identical
                                   ? 1.0f
                                   : diceCoefficient(subtree1, subtree2);
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

    // flatten unmatched trees into parent tree of their roots before doing
    // common distilling
    for (Node *&c : T1->children) {
        if (c->relative == nullptr && c->next != nullptr && !c->next->last) {
            c = c->next;
        }
    }
    for (Node *&c : T2->children) {
        if (c->relative == nullptr && c->next != nullptr && !c->next->last) {
            c = c->next;
        }
    }
    distill(*T1, *T2);

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
