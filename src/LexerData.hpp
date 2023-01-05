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

#ifndef ZOGRASCOPE_LEXERDATA_HPP_
#define ZOGRASCOPE_LEXERDATA_HPP_

#include <cstddef>

#include <string>

class TreeBuilder;

struct LexerData
{
    TreeBuilder *tb;
    const char *contents;
    int tabWidth;

    LexerData(const std::string &str, int tabWidth, TreeBuilder &tb)
        : tb(&tb), contents(str.data()), tabWidth(tabWidth), next(contents),
          finish(str.data() + str.size())
    {
        if (str.empty()) {
            // When there is no input, we want to get just EOF.
            next = nullptr;
        }
    }

    std::size_t readInput(char buf[], std::size_t maxSize);

private:
    const char *next;
    const char *finish;
};

#endif // ZOGRASCOPE_LEXERDATA_HPP_
