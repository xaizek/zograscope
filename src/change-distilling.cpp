#include "change-distilling.hpp"

#include <cmath>

#include <vector>

#include "stypes.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "utils.hpp"

static float childrenSimilarity(const Node *x, const std::vector<Node *> &po1,
                                const Node *y, const std::vector<Node *> &po2);
static bool isTerminal(const Node *n);

namespace {

struct descendants_t {} descendants;
struct subtree_t     {} subtree;

// Represents range of nodes and provides common operations on it.
class NodeRange
{
public:
    // Constructs an empty range.
    NodeRange() : from(-1), to(-1), po(nullptr)
    {
    }

    // Constructs range from all descendants of the node, including the node
    // itself.
    NodeRange(subtree_t, const std::vector<Node *> &po, const Node *n)
        : from(leftmostChild(n)), to(n->poID + 1), po(&po)
    {
    }

    // Constructs range from all descendants of the node, but not including the
    // node itself.
    NodeRange(descendants_t, const std::vector<Node *> &po, const Node *n)
        : from(leftmostChild(n)), to(n->poID), po(&po)
    {
    }

    // List of nodes must be an lvalue.
    NodeRange(subtree_t, std::vector<Node *> &&po, const Node *n) = delete;
    NodeRange(descendants_t, std::vector<Node *> &&po, const Node *n) = delete;

public:
    // Checks whether range includes the node.
    bool includes(const Node *n) const
    {
        return (n->poID >= from && n->poID < to);
    }

    // Computes number of terminals.
    int terminalCount() const
    {
        return (po == nullptr ? 0 : std::count_if(begin(), end(), &isTerminal));
    }

    // Retrieves beginning of the range.
    const Node *const * begin() const
    {
        return (po == nullptr ? nullptr : &(*po)[from]);
    }

    // Retrieves end of the range.
    const Node *const * end() const
    {
        return (po == nullptr ? nullptr : &(*po)[to]);
    }

private:
    // Finds id of the leftmost child of the node ignoring satellites.
    static int leftmostChild(const Node *node)
    {
        for (const Node *child : node->children) {
            if (!child->satellite) {
                return leftmostChild(child);
            }
        }

        return node->poID;
    }

private:
    int from, to;                  // Range defined by from and to ids.
    const std::vector<Node *> *po; // External storage of nodes in post-order.
};

}

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

    if (xParent && xParent->relative && xParent->relative == yParent) {
        return 4;
    }

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
        dice1.emplace_back(x->label);
    }

    std::vector<DiceString> dice2;
    dice2.reserve(po2.size());
    for (Node *x : po2) {
        dice2.emplace_back(x->label);
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

                const float childrenSim = childrenSimilarity(x, po1, y, po2);
                if (childrenSim == 0.0f) {
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

    auto matchPartiallyMatchedInternal = [&](bool excludeValues) {
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

                NodeRange xChildren(descendants, po1, x);
                NodeRange yChildren(descendants, po2, y);

                NodeRange xValue, yValue;
                if (excludeValues && haveValues(x, y)) {
                    xValue = NodeRange(subtree, po1, x->getValue());
                    yValue = NodeRange(subtree, po2, y->getValue());
                }

                int common = 0;
                for (const Node *n : yChildren) {
                    if (!isTerminal(n) || yValue.includes(n)) {
                        continue;
                    }

                    if (n->relative == nullptr) {
                        continue;
                    }

                    if (xChildren.includes(n->relative) &&
                        !xValue.includes(n->relative)) {
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
    // First time around we don't want to use values as our guide because they
    // bind statements too strong, which ruins picking correct value out of
    // several identical candidates.
    matchPartiallyMatchedInternal(true);
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
    matchPartiallyMatchedInternal(false);
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

// Computes children similarity.  Returns the similarity, which is 0.0 if it's
// too small to consider nodes as matching.
static float
childrenSimilarity(const Node *x, const std::vector<Node *> &po1,
                   const Node *y, const std::vector<Node *> &po2)
{
    NodeRange xChildren(descendants, po1, x), yChildren(descendants, po2, y);

    NodeRange xValue, yValue;
    if (haveValues(x, y)) {
        xValue = NodeRange(descendants, po1, x->getValue());
        yValue = NodeRange(descendants, po2, y->getValue());
    }

    // Number of common terminal nodes (terminals of
    // unmatched internal nodes are not ignored).
    int nonValueCommon = 0;
    // Number of selected common terminal nodes (terminals of
    // unmatched internal nodes are ignored).
    int selCommon = 0;

    int yLeaves = 0;
    for (const Node *n : yChildren) {
        if (!isTerminal(n)) {
            continue;
        }
        ++yLeaves;

        if (n->relative == nullptr || !xChildren.includes(n->relative)) {
            continue;
        }

        if (!yValue.includes(n)) {
            ++nonValueCommon;
        }

        const Node *const parent = getParent(n);
        // This might skip children of unmatched internal nodes.
        if (parent == nullptr || parent->relative != nullptr) {
            ++selCommon;
        }
    }

    int xLeaves = xChildren.terminalCount();

    const int xExtra = countSatelliteNodes(x);
    const int yExtra = countSatelliteNodes(y);
    selCommon += std::min(xExtra, yExtra);
    xLeaves += xExtra;
    yLeaves += yExtra;

    const int selMaxLeaves = std::max(xLeaves, yLeaves);
    // Avoid NaN result.  If there are no common leaves, the nodes
    // are the same (XXX: might want to compare satellites in such
    // cases in the future).
    const float childrenSim = selMaxLeaves == 0
                            ? 1.0f
                            : static_cast<float>(selCommon)/selMaxLeaves;

    // Threshold of children similarity depends on number of leaves.
    if (childrenSim >= (std::min(xLeaves, yLeaves) <= 4 ? 0.4f : 0.6f)) {
        return childrenSim;
    }

    xLeaves -= xValue.terminalCount();
    yLeaves -= yValue.terminalCount();

    const int maxLeaves = std::max(xLeaves, yLeaves);
    const float nonValueSim = maxLeaves == 0
                            ? 1.0f
                            : static_cast<float>(nonValueCommon)/maxLeaves;
    if (nonValueSim >= 0.8) {
        return nonValueSim;
    }

    return 0.0f;
}

// Checks whether node is leaf that "matters" (i.e., not a comment).
static bool
isTerminal(const Node *n)
{
    // XXX: should we check for isTravellingNode() instead of just comments?
    return (n->children.empty() && n->type != Type::Comments);
}
