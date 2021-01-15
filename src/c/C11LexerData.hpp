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

#ifndef ZOGRASCOPE__C__LEXERDATA_HPP__
#define ZOGRASCOPE__C__LEXERDATA_HPP__

#include <cstddef>
#include <cstring>

#include <string>

#include "c/c11-parser.gen.hpp"
#include "LexerData.hpp"

struct C11LexerData : LexerData
{
    std::size_t offset = 0U;
    std::size_t lineoffset = 0U;
    std::size_t line = 1U;
    std::size_t col = 1U;

    C11_STYPE startTok = {};
    C11_LTYPE startLoc = {};

    C11ParseData *pd;

    C11LexerData(const std::string &str, TreeBuilder &tb, C11ParseData &pd)
        : LexerData(str, tb), pd(&pd)
    { }
};

#endif // ZOGRASCOPE__C__LEXERDATA_HPP__
