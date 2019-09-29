// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__ALIGN_HPP__
#define ZOGRASCOPE__ALIGN_HPP__

#include "utils/strings.hpp"

#include <deque>
#include <string>
#include <vector>

class Node;

enum class Diff
{
    Left,
    Right,
    Identical,
    Different,
    Fold,
};

struct DiffLine
{
    DiffLine(Diff type, int data = 0) : type(type), data(data)
    { }

    Diff type;
    int data;
};

// Single line information of `DiffSource`.
struct LineInfo
{
    LineInfo() : text(boost::string_ref())
    { }
    LineInfo(boost::string_ref text, const Node *n, const Node *relative)
        : text(text), nodes({ n }), rels({ relative })
    { }

    DiceString text;                 // Unhighlighted line.
    std::vector<const Node *> nodes; // Nodes that appear on this line.
    std::vector<const Node *> rels;  // Relatives of nodes on this line.
};

// Represents tree in a form suitable for diffing.
struct DiffSource
{
    // Formats tokens from the tree without highlighting and collects meta
    // information about every line.
    explicit DiffSource(const Node &root);

    std::vector<LineInfo> lines; // Information about lines.
    std::vector<bool> modified;  // Whether certain line contains changes.

private:
    std::deque<std::string> storage; // Storage that backs the lines.
};

// Generates alignment information describing two sequences.
std::vector<DiffLine> makeDiff(DiffSource &&l, DiffSource &&r);

#endif // ZOGRASCOPE__ALIGN_HPP__
