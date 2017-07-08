#include "compare.hpp"

#include "change-distilling.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static void refine(Node &root);

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
            refine(*subT1);
            t1Child->satellite = true;
            t2Child->satellite = true;
            break;
        }
    }

    distill(*T1, *T2);
    refine(*T1);
}

static void
refine(Node &root)
{
    std::function<void(Node &)> visit = [&](Node &node) {
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
            visit(*child);
        }
    };
    visit(root);
}
