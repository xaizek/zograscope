#include "compare.hpp"

#include "change-distilling.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

void
compare(Node *T1, Node *T2, TimeReport &tr, bool coarse)
{
    tr.measure("coarse-reduction"), reduceTreesCoarse(T1, T2);

    if (!coarse) {
        tr.measure("fine-reduction"), reduceTreesFine(T1, T2);
        tr.measure("diffing"), ted(*T1, *T2);
        return;
    }

    auto timer = tr.measure("reduction-and-diffing");
    for (Node *t1Child : T1->children) {
        if (t1Child->satellite) {
            continue;
        }
        std::string subtree1 = printSubTree(*t1Child);
        for (Node *t2Child : T2->children) {
            if (t2Child->satellite || t1Child->label != t2Child->label ||
                diceCoefficient(subtree1, printSubTree(*t2Child)) < 0.6f) {
                continue;
            }

            Node *subT1 = t1Child, *subT2 = t2Child;
            reduceTreesFine(subT1, subT2);
            distill(*subT1, *subT2);
            t1Child->satellite = true;
            t2Child->satellite = true;
            break;
        }
    }

    distill(*T1, *T2);
}
