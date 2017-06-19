#include "change-distilling.hpp"

#include <vector>

#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static void
postOrder(Node &node, std::vector<Node *> &v, bool excludeSatellites)
{
    node.relative = nullptr;
    if (excludeSatellites && node.satellite) {
        return;
    }

    for (Node &child : node.children) {
        postOrder(child, v, excludeSatellites);
    }
    node.poID = v.size();

    v.push_back(&node);
}

static std::vector<Node *>
postOrder(Node &root, bool excludeSatellites)
{
    std::vector<Node *> v;
    postOrder(root, v, excludeSatellites);
    return v;
}

static bool
canMatch(const Node *x, const Node *y)
{
    if (x->type >= Type::NonInterchangeable ||
        y->type >= Type::NonInterchangeable ||
        x->type != y->type) {
        return false;
    }

    if (x->type == Type::Virtual && x->stype != y->stype) {
        return false;
    }

    return true;
}

static void
markAll(Node &node, State state)
{
    if (node.relative == nullptr) {
        node.state = state;
    }
    for (Node &child : node.children) {
        // markAll(child, state);
        if (child.satellite) {
            child.state = node.state;
        }
    }
}

void
distill(Node &T1, Node &T2)
{
    struct Match
    {
        Node *x;
        Node *y;
        float similarity;
    };

    // std::vector<Node *> po1s = postOrder(T1, false);
    // std::vector<Node *> po2s = postOrder(T2, false);

    std::vector<Node *> po1 = postOrder(T1, true);
    std::vector<Node *> po2 = postOrder(T2, true);
    for (Node *n : po1) {
        n->relative = nullptr;
    }
    for (Node *n : po2) {
        n->relative = nullptr;
    }

    auto commonAreaSize = [&](const Match &m) {
        int size = 1;
        int i = m.x->poID - 1;
        int j = m.y->poID - 1;
        while (i >= 0 && j >= 0 && po1[i]->label == po2[j]->label) {
            ++size;
            --i;
            --j;
        }
        i = m.x->poID + 1;
        j = m.y->poID + 1;
        while (i < (int)po1.size() && j > (int)po2.size() && po1[i]->label == po2[j]->label) {
            ++size;
            ++i;
            ++j;
        }

        return size;
    };

    std::vector<Match> matches;

    for (Node *x : po1) {
        if (!x->children.empty()) {
            continue;
        }

        for (Node *y : po2) {
            if (!y->children.empty()) {
                continue;
            }

            if (!canMatch(x, y)) {
                continue;
            }

            const float similarity = diceCoefficient(x->label, y->label);
            if (similarity >= 0.6f) {
                matches.push_back({ x, y, similarity });
            }
        }
    }

    std::stable_sort(matches.begin(), matches.end(),
                     [&](const Match &a, const Match &b) {
                         if (a.similarity == b.similarity) {
                             return commonAreaSize(b) < commonAreaSize(a);
                         }
                         return b.similarity < a.similarity;
                     });

    for (const Match &match : matches) {
        if (match.x->relative != nullptr || match.y->relative != nullptr) {
            continue;
        }

        match.x->relative = match.y;
        match.y->relative = match.x;

        const State state = (match.similarity == 1.0f)
                          ? State::Unchanged
                          : State::Updated;
        match.x->state = state;
        match.y->state = state;
    }

    std::function<int(const Node *)> lml = [&](const Node *node) {
        if (node->children.empty()) {
            return node->poID;
        }

        for (const Node &child : node->children) {
            if (!child.satellite) {
                return lml(&child);
            }
        }

        return node->poID;
    };

    for (Node *x : po1) {
        if (!x->children.empty() && x->relative == nullptr) {
            for (Node *y : po2) {
                if (!y->children.empty() && y->relative == nullptr) {
                    if (!canMatch(x, y)) {
                        continue;
                    }

                    const int xFrom = lml(x);
                    int common = 0;
                    int yLeaves = 0;
                    for (int i = lml(y); i < y->poID; ++i) {
                        if (!po2[i]->children.empty()) {
                            continue;
                        }
                        ++yLeaves;

                        if (po2[i]->relative == nullptr) {
                            continue;
                        }

                        if (po2[i]->relative->poID >= xFrom &&
                            po2[i]->relative->poID < x->poID) {
                            ++common;
                        }
                    }

                    int xLeaves = 0;
                    for (int i = xFrom; i < x->poID; ++i) {
                        if (po1[i]->children.empty()) {
                            ++xLeaves;
                        }
                    }

                    float t = (std::min(xLeaves, yLeaves) <= 4) ? 0.4f : 0.6f;

                    float similarity2 =
                        static_cast<float>(common)/std::max(xLeaves, yLeaves);
                    if (similarity2 < t) {
                        continue;
                    }

                    float similarity1 = diceCoefficient(x->label, y->label);
                    if (similarity1 < 0.6f && similarity2 < 0.8f) {
                        continue;
                    }

                    State state = (similarity1 == 1.0f && similarity2 == 1.0f)
                                ? State::Unchanged
                                : State::Updated;
                    // markAll(*x, state);
                    // markAll(*y, state);
                    x->state = state;
                    y->state = state;

                    x->relative = y;
                    y->relative = x;

                    break;
                }
            }
        }
    }

    T1.relative = &T2;
    T2.relative = &T1;

    for (Node *x : po1) {
        if (x->relative == nullptr) {
            // x->state = State::Deleted;
            markAll(*x, State::Deleted);
        }
    }
    for (Node *y : po2) {
        if (y->relative == nullptr) {
            // y->state = State::Inserted;
            markAll(*y, State::Inserted);
        }
    }
}
