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

#ifndef ZOGRASCOPE__LEXERDATA_HPP__
#define ZOGRASCOPE__LEXERDATA_HPP__

#include <cstddef>

#include <string>

class TreeBuilder;

struct LexerData
{
    enum { tabWidth = 4 };

    TreeBuilder *tb;

    LexerData(const std::string &str, TreeBuilder &tb)
        : tb(&tb), next(str.data()), finish(str.data() + str.size())
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

#endif // ZOGRASCOPE__LEXERDATA_HPP__
