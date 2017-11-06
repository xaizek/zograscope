#include "change-distilling.hpp"

#include <cmath>

#include <vector>

#include "stypes.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static void
postOrderAndInit(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    node.relative = nullptr;

    for (Node *child : node.children) {
        child->parent = &node;
        postOrderAndInit(*child, v);
    }
    node.poID = v.size();

    v.push_back(&node);
}

static std::vector<Node *>
postOrderAndInit(Node &root)
{
    std::vector<Node *> v;
    root.parent = nullptr;
    postOrderAndInit(root, v);
    return v;
}

static bool
canMatch(const Node *x, const Node *y)
{
    const Type xType = canonizeType(x->type);
    const Type yType = canonizeType(y->type);

    if (xType != Type::Virtual && xType == yType && x->label == y->label) {
        return true;
    }

    if (xType >= Type::NonInterchangeable ||
        yType >= Type::NonInterchangeable ||
        xType != yType) {
        return false;
    }

    if (xType == Type::Virtual && x->stype != y->stype) {
        return false;
    }

    return true;
}

static void
clear(Node *node)
{
    if (node->satellite) {
        return;
    }

    node->relative = nullptr;
    node->state = State::Unchanged;

    for (Node *child : node->children) {
        clear(child);
    }
}

static void
unbind(Node *node)
{
    if (node->children.empty()) {
        return;
    }

    if (Node *relative = node->relative) {
        node->relative = nullptr;
        unbind(relative);
    }

    for (Node *child : node->children) {
        unbind(child);
    }
}

static void
markNode(Node &node, State state)
{
    node.state = state;

    State leafState = (state == State::Updated ? State::Unchanged : state);

    for (Node *child : node.children) {
        child->parent = &node;
        if (child->satellite) {
            if (child->stype == SType::None) {
                child->state = leafState;
            } else if (node.hasValue()) {
                child->state = leafState;
            } else if (child->relative == nullptr) {
                child->state = leafState;
            }
        }
    }
}

static int
countLeaves(const Node *node)
{
    if (node->stype == SType::Separator) {
        return 0;
    }

    if (node->children.empty()) {
        return 1;
    }

    int count = 0;
    for (const Node *child : node->children) {
        count += countLeaves(child);
    }
    return count;
}

static int
countSatelliteNodes(const Node *node)
{
    if (node->satellite) {
        return (node->stype == SType::Separator ? 0 : countLeaves(node));
    }

    int count = 0;
    for (const Node *child : node->children) {
        count += countSatelliteNodes(child);
    }
    return count;
}

static bool
alwaysMatches(const Node *node)
{
    return (node->stype == SType::TranslationUnit);
}

static const Node *
getParent(const Node *n)
{
    const Node *parent = n->parent;
    if (parent != nullptr && isContainer(parent)) {
        return parent->parent;
    }
    return parent;
}

static bool
haveValues(const Node *x, const Node *y)
{
    return x != nullptr
        && y != nullptr
        && x->hasValue()
        && y->hasValue();
}

static int
rateMatch(const Node *x, const Node *y)
{
    const Node *xParent = getParent(x);
    const Node *yParent = getParent(y);

    if (haveValues(xParent, yParent)) {
        if (xParent->getValue()->relative == yParent->getValue()) {
            return 3;
        }
    }

    if ((xParent ? xParent->relative : nullptr) != yParent) {
        return 0;
    }

    if (yParent == nullptr) {
        return xParent == nullptr ? 1 : 0;
    }

    return 2;
}

static void
match(Node *x, Node *y, State state)
{
    markNode(*x, state);
    markNode(*y, state);

    x->relative = y;
    y->relative = x;
}

static bool
unmatchedInternal(Node *node)
{
    return (node->relative == nullptr && !node->children.empty());
}

// This pass matches nodes, whose direct children (ignoring comments) are
// already matched with each other.
static void
matchFirstLevelMatchedInternal(const std::vector<Node *> &po1,
                               const std::vector<Node *> &po2)
{
    for (Node *x : po1) {
        if (!unmatchedInternal(x)) {
            continue;
        }

        for (Node *y : po2) {
            if (!unmatchedInternal(y) || !canMatch(x, y)) {
                continue;
            }

            unsigned int i = 0U, j = 0U;
            int xChildren = 0, yChildren = 0;
            int nMatched = 0;
            while (i < x->children.size() && j < y->children.size()) {
                const Node *xChild = x->children[i], *yChild = y->children[j];
                if (xChild->type == Type::Comments) {
                    ++i;
                    continue;
                }
                if (yChild->type == Type::Comments) {
                    ++j;
                    continue;
                }

                if ((xChild->satellite && yChild->satellite) ||
                    xChild->relative == yChild) {
                    ++nMatched;
                }
                ++i;
                ++j;
                ++xChildren;
                ++yChildren;
            }
            while (i < x->children.size() &&
                   x->children[i]->type == Type::Comments) {
                ++i;
                ++xChildren;
            }
            while (j < y->children.size() &&
                   y->children[j]->type == Type::Comments) {
                ++j;
                ++xChildren;
            }
            if (nMatched == xChildren && nMatched == yChildren) {
                match(x, y, State::Unchanged);
            }
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
        mutable int common;
    };

    std::vector<Node *> po1 = postOrderAndInit(T1);
    std::vector<Node *> po2 = postOrderAndInit(T2);

    std::vector<DiceString> dice1;
    dice1.reserve(po1.size());
    for (Node *x : po1) {
        dice1.push_back(x->label);
    }

    std::vector<DiceString> dice2;
    dice2.reserve(po2.size());
    for (Node *x : po2) {
        dice2.push_back(x->label);
    }

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

            const float similarity = dice1[x->poID].compare(dice2[y->poID]);
            if (similarity >= 0.6f || canForceLeafMatch(x, y)) {
                matches.push_back({ x, y, similarity, -1 });
            }
        }
    }

    std::stable_sort(matches.begin(), matches.end(),
                     [&](const Match &a, const Match &b) {
                         return b.similarity < a.similarity;
                     });

    std::function<int(const Node *)> lml = [&](const Node *node) {
        if (node->children.empty()) {
            return node->poID;
        }

        for (const Node *child : node->children) {
            if (!child->satellite) {
                return lml(child);
            }
        }

        return node->poID;
    };

    auto distillLeafs = [&]() {
        for (const Match &m : matches) {
            if (m.x->relative == nullptr && m.y->relative == nullptr) {
                match(m.x, m.y, (m.similarity == 1.0f &&
                                 m.y->label == m.x->label)
                                ? State::Unchanged
                                : State::Updated);
            }
        }
    };

    auto distillInternal = [&]() {
        for (Node *x : po1) {
            if (!unmatchedInternal(x)) {
                continue;
            }

            for (Node *y : po2) {
                if (!unmatchedInternal(y) || !canMatch(x, y)) {
                    continue;
                }

                if (alwaysMatches(y)) {
                    match(x, y, State::Unchanged);
                    break;
                }

                const Node *xParent = getParent(x);
                const Node *yParent = getParent(y);

                // Containers are there to hold elements of their parent nodes
                // and can be matched only to containers of matched parents.
                if (isContainer(x) && haveValues(xParent, yParent) &&
                    xParent->getValue()->relative != nullptr) {
                    if (xParent->getValue()->relative != yParent->getValue()) {
                        continue;
                    }
                    match(x, y, State::Unchanged);
                    break;
                }

                const int xFrom = lml(x);
                int common = 0;
                int yLeaves = 0;
                for (int i = lml(y); i < y->poID; ++i) {
                    if (!po2[i]->children.empty()) {
                        continue;
                    }
                    if (po2[i]->type == Type::Comments) {
                        continue;
                    }
                    ++yLeaves;

                    const Node *const parent = getParent(po2[i]);
                    if (parent && parent->relative == nullptr) {
                        // Skip children of unmatched internal nodes.
                        continue;
                    }

                    if (po2[i]->relative == nullptr) {
                        continue;
                    }

                    if (po2[i]->relative->poID >= xFrom &&
                        po2[i]->relative->poID < x->poID) {
                        ++common;
                    }
                }

                int xLeaves = std::count_if(&po1[xFrom], &po1[x->poID],
                                            [](const Node *n) {
                                                return n->children.empty() &&
                                                n->type != Type::Comments;
                                            });

                // Threshold of children similarity depends on number of leaves.
                const float t = (std::min(xLeaves, yLeaves) <= 4 ? 0.4f : 0.6f);

                const int xExtra = countSatelliteNodes(x);
                const int yExtra = countSatelliteNodes(y);
                common += std::min(xExtra, yExtra);
                xLeaves += xExtra;
                yLeaves += yExtra;

                const int maxLeaves = std::max(xLeaves, yLeaves);
                const float childrenSim = static_cast<float>(common)/maxLeaves;
                if (childrenSim < t) {
                    continue;
                }

                const float labelSim = dice1[x->poID].compare(dice2[y->poID]);
                if (labelSim < 0.6f && childrenSim < 0.8f) {
                    continue;
                }

                if (labelSim == 1.0f && x->label == y->label &&
                    childrenSim == 1.0f) {
                    match(x, y, State::Unchanged);
                } else {
                    match(x, y, State::Updated);
                }
                break;
            }
        }
    };

    auto matchPartiallyMatchedInternal = [&]() {
        struct Match
        {
            Node *x;
            Node *y;
            int common;
        };

        std::vector<Match> matches;

        // once we have matched internal nodes properly, do second pass matching
        // internal nodes that have at least one common leaf
        for (Node *x : po1) {
            if (!unmatchedInternal(x)) {
                continue;
            }

            for (Node *y : po2) {
                if (!unmatchedInternal(y) || !canMatch(x, y)) {
                    continue;
                }

                const int xFrom = lml(x);
                int common = 0;
                for (int i = lml(y); i < y->poID; ++i) {
                    if (!po2[i]->children.empty()) {
                        continue;
                    }

                    if (po2[i]->relative == nullptr) {
                        continue;
                    }

                    if (po2[i]->relative->poID >= xFrom &&
                        po2[i]->relative->poID < x->poID) {
                        ++common;
                    }
                }

                const float similarity = dice1[x->poID].compare(dice2[y->poID]);
                if (common > 0 && similarity >= 0.5f) {
                    matches.push_back({ x, y, common });
                }
            }
        }

        std::stable_sort(matches.begin(), matches.end(),
                        [&](const Match &a, const Match &b) {
                            return b.common < a.common;
                        });

        for (const Match &m : matches) {
            if (m.x->relative == nullptr && m.y->relative == nullptr) {
                match(m.x, m.y, State::Unchanged);
            }
        }
    };

    distillLeafs();
    distillInternal();
    matchPartiallyMatchedInternal();
    matchFirstLevelMatchedInternal(po1, po2);

    std::stable_sort(matches.begin(), matches.end(),
                     [&](const Match &a, const Match &b) {
                         if (std::fabs(a.similarity - b.similarity) < 0.01f) {
                             return rateMatch(b.x, b.y) < rateMatch(a.x, a.y);
                         }
                         return b.similarity < a.similarity;
                     });

    clear(&T1);
    clear(&T2);

    distillLeafs();
    distillInternal();
    matchPartiallyMatchedInternal();
    matchFirstLevelMatchedInternal(po1, po2);

    for (Node *x : po1) {
        if (x->relative == nullptr) {
            markNode(*x, State::Deleted);
        }
    }
    for (Node *y : po2) {
        if (y->relative == nullptr) {
            markNode(*y, State::Inserted);
        }
    }
}
