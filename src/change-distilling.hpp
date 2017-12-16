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

class Node;

// Implements change-distilling algorithm.
class Distiller
{
public:
    // Computes changes between two disjoint subtrees and marks nodes
    // appropriately.
    void distill(Node &T1, Node &T2);

private:
    std::vector<Node *> po1, po2; // Nodes in post-order traversal order.
};

#endif // ZOGRASCOPE__CHANGE_DISTILLING_HPP__
