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

#ifndef ZOGRASCOPE_TREE_EDIT_DISTANCE_HPP_
#define ZOGRASCOPE_TREE_EDIT_DISTANCE_HPP_

#include <string>

class Node;
class Tree;

void printTree(const std::string &name, Tree &tree);

int ted(Node &T1, Node &T2);

#endif // ZOGRASCOPE_TREE_EDIT_DISTANCE_HPP_
