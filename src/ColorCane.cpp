// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "ColorCane.hpp"

#include <string>
#include <utility>
#include <vector>

void
ColorCane::append(boost::string_ref text, const Node *node, ColorGroup hi)
{
    if (canAppend(node, hi)) {
        pieces.back().text.append(text.begin(), text.end());
    } else {
        pieces.emplace_back(text.to_string(), node, hi);
    }
}

void
ColorCane::append(char text, ColorGroup hi, const Node *node)
{
    if (canAppend(node, hi)) {
        pieces.back().text.append(1, text);
    } else {
        pieces.emplace_back(std::string(1, text), node, hi);
    }
}

void
ColorCane::append(char text, int count, ColorGroup hi, const Node *node)
{
    if (canAppend(node, hi)) {
        pieces.back().text.append(count, text);
    } else {
        pieces.emplace_back(std::string(count, text), node, hi);
    }
}

bool
ColorCane::canAppend(const Node *node, ColorGroup hi) const
{
    return !pieces.empty()
        && pieces.back().node == node
        && pieces.back().hi == hi;
}

auto
ColorCane::begin() const -> Pieces::const_iterator
{
    return pieces.begin();
}

auto
ColorCane::end() const -> Pieces::const_iterator
{
    return pieces.end();
}

std::vector<ColorCane>
ColorCane::splitIntoLines() &&
{
    std::vector<ColorCane> split(1);
    for (ColorCanePiece &piece : pieces) {
        std::string &text = piece.text;

        while (!text.empty()) {
            auto pos = text.find('\n');
            if (pos == std::string::npos) {
                split.back().append(std::move(text), piece.node, piece.hi);
                break;
            }

            if (pos != 0U) {
                split.back().append(std::string(text, 0, pos), piece.node,
                                    piece.hi);
            }
            text.erase(0, pos + 1);
            split.emplace_back();
        }
    }
    return split;
}

std::vector<ColorCane>
ColorCane::breakAt(boost::string_ref separators) &&
{
    std::vector<ColorCane> split(1);
    for (ColorCanePiece &piece : pieces) {
        std::string &text = piece.text;

        if (split.size() == 2U) {
            split.back().append(std::move(text), piece.node, piece.hi);
            continue;
        }

        auto pos = text.find_first_not_of(separators.data(), 0U,
                                          separators.size());
        if (pos == std::string::npos) {
            split.back().append(std::move(text), piece.node, piece.hi);
            continue;
        }

        if (pos != 0U) {
            split.back().append(std::string(text, 0, pos), piece.node,
                                piece.hi);
        }
        text.erase(0, pos);
        split.emplace_back();
        split.back().append(std::move(text), piece.node, piece.hi);
    }

    if (split.size() == 1U) {
        split.emplace_back();
    }

    return split;
}
