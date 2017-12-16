// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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

#include <vector>

class Language;
class Node;

// Implements change-distilling algorithm.
class Distiller
{
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
    // Computes children similarity.  Returns the similarity, which is 0.0 if
    // it's too small to consider nodes as matching.
    float childrenSimilarity(const Node *x,
                             const std::vector<Node *> &po1,
                             const Node *y,
                             const std::vector<Node *> &po2) const;
    // Computes rating of a match, which is to be compared with ratings of other
    // matches.
    int rateMatch(const Node *x, const Node *y) const;
    // Retrieves parent of the node possibly skipping container parents.  Might
    // return `nullptr`.
    const Node * getParent(const Node *n) const;

private:
    Language &lang;               // Language of the nodes.
    std::vector<Node *> po1, po2; // Nodes in post-order traversal order.
};

#endif // ZOGRASCOPE__CHANGE_DISTILLING_HPP__
