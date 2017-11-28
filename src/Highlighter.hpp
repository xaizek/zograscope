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

#ifndef ZOGRASCOPE__HIGHLIGHTER_HPP__
#define ZOGRASCOPE__HIGHLIGHTER_HPP__

#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <boost/utility/string_ref.hpp>

class Node;

class Highlighter
{
    class ColorPicker;

public:
    Highlighter(Node &root, bool original = true);

    Highlighter(const Highlighter&) = delete;
    Highlighter & operator=(const Highlighter&) = delete;

    ~Highlighter();

public:
    std::string print(int from, int n);
    std::string print();

private:
    void skipUntil(int targetLine);
    void print(int n);
    void printSpelling(int &n);
    Node * getNode();
    void advanceNode(Node *node);

private:
    std::ostringstream oss;
    int line, col;
    std::unique_ptr<ColorPicker> colorPicker;
    std::vector<boost::string_ref> olines;
    std::vector<boost::string_ref> lines;
    std::stack<Node *> toProcess;
    std::string spelling;
    bool original;
};

#endif // ZOGRASCOPE__HIGHLIGHTER_HPP__
