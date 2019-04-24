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

#ifndef ZOGRASCOPE__MAKE__MAKELEXERDATA_HPP__
#define ZOGRASCOPE__MAKE__MAKELEXERDATA_HPP__

#include <cstddef>
#include <cstring>

#include <string>
#include <vector>

#include "make/make-parser.hpp"
#include "LexerData.hpp"

// Make-specific state of the lexer.
struct MakeLexerData : LexerData
{
    // Type of nesting.
    enum {
        FunctionNesting, // Function nesting.
        ArgumentNesting, // Function argument nesting.
    };

    std::size_t offset = 0U; // Byte offset in the input.
    std::size_t line = 1U;   // Current line number.
    std::size_t col = 1U;    // Current column number.

    // Start token for things like comments and literals.
    MAKE_STYPE startTok = {};
    // Start location for things like comments and literals.
    MAKE_LTYPE startLoc = {};

    // Offset of the last token that was returned by the lexer.
    std::size_t lastReturnedOffset = static_cast<std::size_t>(-1);
    // Whether whitespace token might be needed before the next token.
    bool fakeWSIsNeeded = false;
    // Keeps track of function nesting (where keywords stop being keywords).
    std::vector<bool> nesting;

    MakeParseData *pd; // Link to Make-specific state of the parser.

    // Remembers arguments to use them in the lexer, all of them must be alive
    // during lexing (including the string which isn't copied).
    MakeLexerData(const std::string &str, TreeBuilder &tb, MakeParseData &pd)
        : LexerData(str, tb), pd(&pd)
    { }
};

#endif // ZOGRASCOPE__MAKE__MAKELEXERDATA_HPP__
