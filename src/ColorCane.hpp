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

#ifndef ZOGRASCOPE_COLORCANE_HPP_
#define ZOGRASCOPE_COLORCANE_HPP_

#include <boost/utility/string_ref.hpp>

#include <string>
#include <utility>
#include <vector>

enum class ColorGroup;

class Node;

// Single item of `ColorCane`.
struct ColorCanePiece
{
    ColorCanePiece(std::string text, const Node *node, ColorGroup hi)
        : text(std::move(text)), node(node), hi(hi)
    { }

    std::string text; // Text of the item (can be empty).
    const Node *node; // Node associated with the text (can be `nullptr`).
    ColorGroup hi;    // Highlighting of the piece.
};

// Allows constructing string consisting of multiple pieces each of which is
// associated with some metadata.
class ColorCane
{
    using Pieces = std::vector<ColorCanePiece>;

public:
    // Appends a string.
    void append(boost::string_ref text, const Node *node, ColorGroup hi = {});
    // Appends single character.
    void append(char text, ColorGroup hi = {}, const Node *node = nullptr);
    // Appends single character that's repeated `count` times.
    void append(char text, int count, ColorGroup hi = {},
                const Node *node = nullptr);

    // Beginning of the list of pieces.
    Pieces::const_iterator begin() const;
    // End of the list of pieces.
    Pieces::const_iterator end() const;

    // Breaks the cane into possibly multiple ones none of which contain `'\n'`.
    std::vector<ColorCane> splitIntoLines() &&;
    // Breaks the cane into exactly two ones at first character not in
    // `separators`.
    std::vector<ColorCane> cutAfter(boost::string_ref separators) &&;

private:
    // Checks whether can append to the last piece instead of creating new one.
    bool canAppend(const Node *node, ColorGroup hi) const;

private:
    Pieces pieces; // List of pieces.
};

#endif // ZOGRASCOPE_COLORCANE_HPP_
