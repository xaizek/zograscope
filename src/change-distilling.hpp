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

#ifndef ZOGRASCOPE__CHANGE_DISTILLING_HPP__
#define ZOGRASCOPE__CHANGE_DISTILLING_HPP__

#include <cstdint>

#include <vector>

enum class State : std::uint8_t;

class DiceString;
class Language;
class Node;

enum class OverlapKind;

// Implements change-distilling algorithm.
class Distiller
{
    struct TerminalMatch;

public:
    // Creates an instance for the specific language.
    Distiller(Language &lang) : lang(lang)
    {
    }

public:
    // Computes changes between two disjoint subtrees and marks nodes
    // appropriately.
    void distill(Node &T1, Node &T2);

private:
    // Initializes {po,dice}[12] fields.
    void initialize(Node &T1, Node &T2);
    // Composes list of viable matches of terminals.
    std::vector<TerminalMatch> generateTerminalMatches();
    // Computes children similarity.  Returns the similarity, which is 0.0 if
    // it's too small to consider nodes as matching.
    float childrenSimilarity(const Node *x,
                             const std::vector<Node *> &po1,
                             const Node *y,
                             const std::vector<Node *> &po2) const;
    // Computes rating of a match of terminals, which is to be compared with
    // ratings of other matches.
    int rateTerminalsMatch(const Node *x, const Node *y) const;
    // Computes rate that depends on number and position of neighbouring nodes
    // of `x` that match corresponding (by offset) nodes of `y`.  This
    // heuristics glues unmatched nodes to their already matched neighbours and
    // resolves ties quite well.  Matched nodes that are closer to the one
    // being analyzed contribute more to the rate.
    int rateOverlap(const Node *x, const Node *y, OverlapKind how) const;
    // Retrieves parent of the node possibly skipping container parents.  Might
    // return `nullptr`.
    const Node * getParent(const Node *n) const;
    // Counts number of already matched elements in specified subtree.
    int countAlreadyMatched(const Node *node) const;
    // Counts number of already matched leaves in specified subtree.
    int countAlreadyMatchedLeaves(const Node *node) const;
    // Main pass for matching internal nodes.
    void distillInternal();
    // Matches unmatched internal nodes with similar nodes that have maximum
    // number of common terminal nodes.
    void matchPartiallyMatchedInternal(bool excludeValues);
    // This pass matches nodes, whose direct children (ignoring comments) are
    // already matched with each other.
    void matchFirstLevelMatchedInternal();
    // Applies matching to terminals.
    void applyTerminalMatches(const std::vector<TerminalMatch> &matches);
    // Changes state of two nodes and connects them.
    void match(Node *x, Node *y, State state);

private:
    Language &lang;                // Language of the nodes.
    std::vector<Node *> po1, po2;  // Nodes in post-order traversal order.
    std::vector<DiceString> dice1; // DiceString of corresponding po1[i]->label.
    std::vector<DiceString> dice2; // DiceString of corresponding po2[i]->label.
};

#endif // ZOGRASCOPE__CHANGE_DISTILLING_HPP__
