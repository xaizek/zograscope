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

#include "LexerData.hpp"

#include <cstddef>
#include <cstring>

#include <algorithm>

std::size_t
LexerData::readInput(char buf[], std::size_t maxSize)
{
    static const char *const trailing = "\n";

    if (next == nullptr) {
        return 0U;
    }

    std::size_t count = std::min<std::size_t>(finish - next, maxSize);
    char *end = std::copy_n(next, count, buf);
    const std::size_t copied = end - buf;

    if (next[copied] == '\0') {
        next = (next == trailing ? nullptr : trailing);
        finish = (next == nullptr ? nullptr : next + std::strlen(next));
    } else {
        next += copied;
    }

    return copied;
}
