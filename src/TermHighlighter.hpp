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

#ifndef ZOGRASCOPE__TERMHIGHLIGHTER_HPP__
#define ZOGRASCOPE__TERMHIGHLIGHTER_HPP__

#include <string>

#include "ColorScheme.hpp"
#include "Highlighter.hpp"

// Tree highlighter that produces strings suitable for printing on a terminal.
// Highlights either all at once or by line ranges.
class TermHighlighter : private Highlighter
{
public:
    using Highlighter::Highlighter;

public:
    using Highlighter::setPrintReferences;
    using Highlighter::setPrintBrackets;
    using Highlighter::setTransparentDiffables;

    // Prints lines in the range [from, from + n) into a string.  Each line can
    // be printed at most once, thus calls to this function need to increase
    // `from` argument.  Returns the string.
    std::string print(int from, int n);
    // Prints lines until the end into a string.  Returns the string.
    std::string print();

private:
    ColorScheme cs; // Terminal color scheme.
};

#endif // ZOGRASCOPE__TERMHIGHLIGHTER_HPP__
