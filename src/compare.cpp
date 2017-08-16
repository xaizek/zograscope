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

    tr.measure("coarse-reduction"), reduceTreesCoarse(T1, T2);

    if (!coarse) {
        tr.measure("fine-reduction"), reduceTreesFine(T1, T2);
        tr.measure("diffing"), ted(*T1, *T2);
        return;
    }

    auto refine = [skipRefine](Node &root) {
        if (!skipRefine) {
            ::refine(root);
        }
    };

    std::vector<Match> matches;

    auto timer = tr.measure("reduction-and-diffing");
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
        if (match.x->satellite || match.y->satellite) {
            continue;
        }

        Node *subT1 = match.x, *subT2 = match.y;
        reduceTreesFine(subT1, subT2);
        distill(*subT1, *subT2);
        refine(*subT1);
        match.x->satellite = true;
        match.y->satellite = true;
    }

    distill(*T1, *T2);
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
        reduceTreesFine(subT1, subT2);
        ted(*subT1, *subT2);
    }

    for (Node *child : node.children) {
        refine(*child);
    }
}
