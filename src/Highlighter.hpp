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

#include <cstdint>

#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <boost/utility/string_ref.hpp>

#include "ColorScheme.hpp"

enum class State : std::uint8_t;

class Language;
class Node;
class Tree;

// Tree highlighter.  Highlights either all at once or by line ranges.
class Highlighter
{
    class ColorPicker;
    class Entry;

public:
    // Stores arguments for future reference.  The original flag specifies
    // whether this is an old version of a file (matters only for trees marked
    // with results of comparison).
    Highlighter(const Tree &tree, bool original = true);

    // Tree can't be a temporary.
    Highlighter(Tree &&tree, bool original = true) = delete;

    // Stores arguments for future reference.  The original flag specifies
    // whether this is an old version of a file (matters only for trees marked
    // with results of comparison).
    Highlighter(const Node &root, const Language &lang, bool original = true,
                int lineOffset = 1);

    // No copying.
    Highlighter(const Highlighter&) = delete;
    // No assigning.
    Highlighter & operator=(const Highlighter&) = delete;

    // Destructs the highlighter.
    ~Highlighter();

private:
    // Common implementation of two public constructors.
    Highlighter(const Node &root, const Language &lang, bool original,
                int lineOffset, int colOffset);

public:
    // Prints lines in the range [from, from + n) into a string.  Each line can
    // be printed at most once, thus calls to this function need to increase
    // from argument.  Returns the string.
    std::string print(int from, int n);
    // Prints lines until the end into a string.  Returns the string.
    std::string print();

private:
    // Skips everything until target line is reached.
    void skipUntil(int targetLine);
    // Prints at most `n` lines.
    void print(int n);
    // Prints lines of spelling decreasing `n` on advancing through lines.
    void printSpelling(int &n);
    // Retrieves the next entry to be processed.
    Entry getEntry();
    // Advances processing to the next node.  The entry here is the one that was
    // returned by `getEntry()` earlier.
    void advance(const Entry &entry);
    // Formats spelling of a node into a colored string.
    std::string getSpelling(const Node &node, State state, ColorGroup def);
    // Diffs labels of two nodes (specified one and its relative).  Unchanged
    // parts are highlighted using `dec`.
    std::string diffSpelling(const Node &node, ColorGroup def);

private:
    const Language &lang;                     // Language services.
    std::ostringstream oss;                   // Temporary output buffer.
    int line, col;                            // Current position.
    int colOffset;                            // Horizontal offset;
    std::unique_ptr<ColorPicker> colorPicker; // Highlighting state.
    std::vector<boost::string_ref> olines;    // Undiffed spelling.
    std::vector<boost::string_ref> lines;     // Possibly diffed spelling.
    std::stack<Entry> toProcess;              // State of tree traversal.
    std::string spelling;                     // Storage behind `lines` field.
    bool original;                            // Whether this is an old version.
    const Node *current;                      // Node that's being processed.
    ColorScheme cs;                           // Terminal color scheme.
};

#endif // ZOGRASCOPE__HIGHLIGHTER_HPP__
