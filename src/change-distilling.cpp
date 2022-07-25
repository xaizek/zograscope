// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "change-distilling.hpp"

#include <cmath>

#include <algorithm>
#include <vector>

#include "utils/strings.hpp"
#include "Language.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"

static void postOrderAndInit(Node &root, std::vector<Node *> &v);
static void postOrderAndInitImpl(Node &node, std::vector<Node *> &v);
static void clear(Node *node);
static bool haveValues(const Node *x, const Node *y);
static bool unmatchedInternal(const Node *node);
static bool canMatch(const Node *x, const Node *y);
static bool isTerminal(const Node *n);
static void markNode(Node &node, State state);

// How many neighbours to consider on each side when computing overlap.
static const int TerminalOverlapSize = 3;

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

// Method used to determine if two nodes match on overlap.
enum class OverlapKind
{
    Relation, // The nodes were matched with each other.
    Token,    // The spelling of nodes matches.
};

// Description of a single match candidate for matching terminals.
struct Distiller::TerminalMatch
{
    Node *x;            // Node of the first tree (T1).
    Node *y;            // Node of the first tree (T2).
    float similarity;   // How similar labels of two nodes are in [0.0, 1.0].
};

static bool
isAnOverlap(const Node *x, const Node *y, OverlapKind how)
{
    switch (how) {
        case OverlapKind::Relation: return (x->relative == y);
        case OverlapKind::Token:    return (x->label == y->label);
    }
    return false;
}

int
Distiller::rateOverlap(const Node *x, const Node *y, OverlapKind how) const
{
    int overlap = 0;

    int maxLeftOffset = std::min({ x->poID, y->poID, TerminalOverlapSize });
    for (int i = 1; i <= maxLeftOffset; ++i) {
        int xi = x->poID - i;
        int yi = y->poID - i;
        if (isAnOverlap(po1[xi], po2[yi], how)) {
            overlap += maxLeftOffset - i + 1;
        }
    }

    int maxRightOffset = std::min({ static_cast<int>(po1.size()) - 1 - x->poID,
                                    static_cast<int>(po2.size()) - 1 - y->poID,
                                    TerminalOverlapSize });
    for (int i = 1; i <= maxRightOffset; ++i) {
        int xi = x->poID + i;
        int yi = y->poID + i;
        if (isAnOverlap(po1[xi], po2[yi], how)) {
            overlap += maxRightOffset - i + 1 + (xi == yi);
        }
    }

    return overlap;
}

int
Distiller::rateTerminalsMatch(const Node *x, const Node *y) const
{
    const Node *xParent = getParent(x);
    const Node *yParent = getParent(y);

    if (xParent && xParent->relative && xParent->relative == yParent) {
        return 4 + rateOverlap(x, y, OverlapKind::Relation);
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

void
Distiller::distill(Node &T1, Node &T2)
{
    initialize(T1, T2);

    std::vector<TerminalMatch> matches;

    // First time terminal matching.
    matches = generateTerminalMatches();
    std::stable_sort(matches.begin(), matches.end(),
                     [&](const TerminalMatch &a, const TerminalMatch &b) {
                         // Use overlap rate here too to resolve ties, but
                         // match token-by-token rather then relations, which
                         // don't exist yet.
                         //
                         // The idea is to get fewer incorrect satellite matches
                         // which otherwise stick and kinda ruin everything.

                         // TODO: cache the rate (and not just here)
                         if (b.similarity == a.similarity) {
                            return rateOverlap(b.x, b.y, OverlapKind::Token)
                                 < rateOverlap(a.x, a.y, OverlapKind::Token);
                         }
                         return b.similarity < a.similarity;
                     });
    applyTerminalMatches(matches);

    distillInternal();
    // First time around we don't want to use values as our guide because they
    // bind statements too strongly, which ruins picking correct value out of
    // several identical candidates.
    matchPartiallyMatchedInternal(true);
    matchFirstLevelMatchedInternal();

    // Second round.

    // Terminal re-matching.
    std::stable_sort(matches.begin(), matches.end(),
                     [&](const TerminalMatch &a, const TerminalMatch &b) {
                         if (std::fabs(a.similarity - b.similarity) < 0.01f) {
                             return rateTerminalsMatch(b.x, b.y)
                                  < rateTerminalsMatch(a.x, a.y);
                         }
                         return b.similarity < a.similarity;
                     });
    clear(&T1);
    clear(&T2);
    applyTerminalMatches(matches);

    distillInternal();
    matchPartiallyMatchedInternal(false);
    matchFirstLevelMatchedInternal();

    // Marking remaining unmatched nodes.
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

void
Distiller::initialize(Node &T1, Node &T2)
{
    postOrderAndInit(T1, po1);
    postOrderAndInit(T2, po2);

    dice1.clear();
    dice1.reserve(po1.size());
    for (Node *x : po1) {
        dice1.emplace_back(x->label);
    }

    dice2.clear();
    dice2.reserve(po2.size());
    for (Node *x : po2) {
        dice2.emplace_back(x->label);
    }
}

// Initializes nodes state preparing them for comparison and fills `v` with
// pointers to nodes in post-order.
static void
postOrderAndInit(Node &root, std::vector<Node *> &v)
{
    root.parent = nullptr;
    v.clear();
    postOrderAndInitImpl(root, v);
}

// Build list of nodes in post-order initializing their `parent`, `relative` and
// `poID` fields on the way.
static void
postOrderAndInitImpl(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    node.relative = nullptr;

    for (Node *child : node.children) {
        child->parent = &node;
        postOrderAndInitImpl(*child, v);
    }
    node.poID = v.size();

    v.push_back(&node);
}

// Resets `relative` and `state` fields of non-satellite nodes within a subtree.
static void
clear(Node *node)
{
    // Treat layer breaks as satellites.
    if (node->satellite || node->next != nullptr) {
        return;
    }

    node->relative = nullptr;
    node->state = State::Unchanged;

    for (Node *child : node->children) {
        clear(child);
    }
}

std::vector<Distiller::TerminalMatch>
Distiller::generateTerminalMatches()
{
    std::vector<TerminalMatch> matches;

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
                matches.push_back({ x, y, similarity });
            }
        }
    }

    return matches;
}

float
Distiller::childrenSimilarity(const Node *x,
                              const std::vector<Node *> &po1,
                              const Node *y,
                              const std::vector<Node *> &po2) const
{
    NodeRange xChildren(descendants, po1, x), yChildren(descendants, po2, y);

    NodeRange xValue, yValue;
    if (haveValues(x, y)) {
        xValue = NodeRange(descendants, po1, x->getValue());
        yValue = NodeRange(descendants, po2, y->getValue());
    }

    // Number of common terminal nodes (terminals of unmatched internal nodes
    // are not ignored).
    int nonValueCommon = 0;
    // Number of selected common terminal nodes (terminals of unmatched internal
    // nodes are ignored).
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

    const int xExtra = countAlreadyMatched(x);
    const int yExtra = countAlreadyMatched(y);
    selCommon += std::min(xExtra, yExtra);
    xLeaves += xExtra;
    yLeaves += yExtra;

    const int selMaxLeaves = std::max(xLeaves, yLeaves);
    // Avoid NaN result.  If there are no common leaves, the nodes are the same
    // (XXX: might want to compare satellites in such cases in the future).
    const float childrenSim = selMaxLeaves == 0
                            ? 1.0f
                            : static_cast<float>(selCommon)/selMaxLeaves;

    // Threshold of children similarity depends on number of leaves.
    if (childrenSim >= (std::min(xLeaves, yLeaves) <= 4 ? 0.4f : 0.6f)) {
        return childrenSim;
    }

    // Disregard values only if they aren't matched.
    if (haveValues(x, y) && x->getValue()->relative == nullptr &&
        y->getValue()->relative == nullptr) {
        xLeaves -= xValue.terminalCount();
        yLeaves -= yValue.terminalCount();

        const int maxLeaves = std::max(xLeaves, yLeaves);
        const float nonValueSim = maxLeaves == 0
                                ? 1.0f
                                : static_cast<float>(nonValueCommon)/maxLeaves;
        if (nonValueSim >= 0.8) {
            return nonValueSim;
        }
    }

    return 0.0f;
}

const Node *
Distiller::getParent(const Node *n) const
{
    const Node *parent = n->parent;
    if (parent != nullptr && lang.isContainer(parent)) {
        return parent->parent;
    }
    return parent;
}

int
Distiller::countAlreadyMatched(const Node *node) const
{
    if (node->satellite) {
        return countAlreadyMatchedLeaves(node);
    }

    int count = 0;
    for (const Node *child : node->children) {
        count += countAlreadyMatched(child);
    }
    return count;
}

int
Distiller::countAlreadyMatchedLeaves(const Node *node) const
{
    if (lang.isSatellite(node->stype)) {
        // We aren't interested in satellites specified by the language, because
        // they don't participate in comparison.
        return 0;
    }

    if (node->children.empty()) {
        // We get here only for descendants of non-language-specific satellites
        // that are themselves aren't such satellites, so count the node.
        return 1;
    }

    int count = 0;
    for (const Node *child : node->children) {
        count += countAlreadyMatchedLeaves(child);
    }
    return count;
}

void
Distiller::distillInternal()
{
    for (Node *x : po1) {
        if (!unmatchedInternal(x)) {
            continue;
        }

        for (Node *y : po2) {
            if (!unmatchedInternal(y) || !canMatch(x, y)) {
                continue;
            }

            if (lang.alwaysMatches(y)) {
                match(x, y, State::Unchanged);
                break;
            }

            const Node *xParent = getParent(x);
            const Node *yParent = getParent(y);

            // Containers are there to hold elements of their parent nodes
            // and can be matched only to containers of matched parents.
            if (lang.isContainer(x) && haveValues(xParent, yParent) &&
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
}

void
Distiller::matchPartiallyMatchedInternal(bool excludeValues)
{
    // Description of a single match candidate.
    struct Match
    {
        Node *x;             // Node of the first tree (T1).
        Node *y;             // Node of the second tree (T2).
        int common;          // Number of common terminal nodes, either with
                             // or without value nodes.
        int commonWithValue; // Number of common terminal nodes including
                             // children of value nodes.  Used to resolve
                             // ties on `common`.
    };

    std::vector<Match> matches;

    // Once we have matched internal nodes properly, do second pass matching
    // internal nodes that have at least one common leaf.
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
            if (haveValues(x, y)) {
                xValue = NodeRange(subtree, po1, x->getValue());
                yValue = NodeRange(subtree, po2, y->getValue());
            }

            int common = 0;
            int commonWithValue = 0;
            for (const Node *n : yChildren) {
                if (!isTerminal(n)) {
                    continue;
                }

                if (n->relative == nullptr) {
                    continue;
                }

                if (xChildren.includes(n->relative)) {
                    if (!yValue.includes(n) &&
                        !xValue.includes(n->relative)) {
                        ++common;
                    }
                    ++commonWithValue;
                }
            }

            if (!excludeValues) {
                common = commonWithValue;
            }

            const float similarity = dice1[x->poID].compare(dice2[y->poID]);
            if (common > 0 && similarity >= 0.5f) {
                matches.push_back({ x, y, common, commonWithValue });
            }
        }
    }

    std::stable_sort(matches.begin(), matches.end(),
                    [&](const Match &a, const Match &b) {
                        return b.common < a.common
                            || (b.common == a.common &&
                                b.commonWithValue < a.commonWithValue);
                    });

    for (const Match &m : matches) {
        if (m.x->relative == nullptr && m.y->relative == nullptr) {
            match(m.x, m.y, State::Unchanged);
        }
    }
}

// Checks whether both nodes are non-null and have values (null-nodes obviously
// can't contain values).
static bool
haveValues(const Node *x, const Node *y)
{
    return x != nullptr
        && y != nullptr
        && x->hasValue()
        && y->hasValue();
}

void
Distiller::matchFirstLevelMatchedInternal()
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

// Checks whether node was not yet matched and is not a terminal.
static bool
unmatchedInternal(const Node *node)
{
    // XXX: children here might include satellites, is this OK?
    return (node->relative == nullptr && !node->children.empty());
}

// Checks whether two nodes can match each other.
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

// Checks whether node is leaf that "matters" (i.e., not a comment).
static bool
isTerminal(const Node *n)
{
    // XXX: children here might include satellites, is this OK?
    // XXX: should we check for isTravellingNode() instead of just comments?
    return (n->children.empty() && n->type != Type::Comments);
}

void
Distiller::applyTerminalMatches(const std::vector<TerminalMatch> &matches)
{
    for (const TerminalMatch &m : matches) {
        if (m.x->relative == nullptr && m.y->relative == nullptr) {
            match(m.x, m.y, (m.similarity == 1.0f && m.y->label == m.x->label)
                            ? State::Unchanged
                            : State::Updated);
        }
    }
}

void
Distiller::match(Node *x, Node *y, State state)
{
    auto isSimilarTree = [&](Node *x, Node *y) {
        auto l = x->children.begin();
        auto r = y->children.begin();
        int satellites = 0;
        while (true) {
            while (l != x->children.end() && !lang.isSatellite((*l)->stype)) {
                ++l;
            }
            while (r != y->children.end() && !lang.isSatellite((*r)->stype)) {
                ++r;
            }

            if ((l == x->children.end()) != (r == y->children.end())) {
                return false;
            }
            if (l == x->children.end()) {
                break;
            }

            if ((*l)->stype != (*r)->stype || (*l)->label != (*r)->label) {
                return false;
            }

            ++satellites;
            ++l;
            ++r;
        }
        return (satellites > 0);
    };

    if (isSimilarTree(x, y)) {
        for (auto l = x->children.begin(); l != x->children.end(); ++l) {
            (*l)->parent = x;
        }
        for (auto r = y->children.begin(); r != y->children.end(); ++r) {
            (*r)->parent = y;
        }

        auto l = x->children.begin();
        auto r = y->children.begin();
        while (true) {
            while (l != x->children.end() && !lang.isSatellite((*l)->stype)) {
                ++l;
            }
            while (r != y->children.end() && !lang.isSatellite((*r)->stype)) {
                ++r;
            }

            if (l == x->children.end()) {
                break;
            }

            (*l)->state = State::Unchanged;
            (*r)->state = State::Unchanged;

            (*l)->relative = *r;
            (*r)->relative = *l;

            ++l;
            ++r;
        }

        x->state = state;
        y->state = state;
        x->relative = y;
        y->relative = x;
        return;
    }

    markNode(*x, state);
    markNode(*y, state);

    x->relative = y;
    y->relative = x;
}

// Marks node and its immediate children with the specified state.
static void
markNode(Node &node, State state)
{
    node.state = state;

    State leafState = (state == State::Updated ? State::Unchanged : state);

    for (Node *child : node.children) {
        child->parent = &node;
        if (child->satellite) {
            if (child->stype == SType{}) {
                child->state = leafState;
            } else if (node.hasValue()) {
                child->state = leafState;
            } else if (child->relative == nullptr) {
                child->state = leafState;
            }
        }
    }
}
