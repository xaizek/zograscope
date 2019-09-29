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

#include "TermHighlighter.hpp"

#include <sstream>
#include <string>

#include "ColorCane.hpp"

std::string
TermHighlighter::print(int from, int n)
{
    std::ostringstream oss;
    for (const ColorCanePiece &piece : Highlighter::print(from, n)) {
        oss << (cs[piece.hi] << piece.text);
    }
    return oss.str();
}

std::string
TermHighlighter::print()
{
    std::ostringstream oss;
    for (const ColorCanePiece &piece : Highlighter::print()) {
        oss << (cs[piece.hi] << piece.text);
    }
    return oss.str();
}
