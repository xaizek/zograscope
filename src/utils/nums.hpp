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

#ifndef ZOGRASCOPE_UTILS_NUMS_HPP_
#define ZOGRASCOPE_UTILS_NUMS_HPP_

#include <cassert>

// Computes number of digits in a positive number (including zero).
inline int
countWidth(int n)
{
    assert(n >= 0 && "Argument must be >= 0.");

    int width = 0;
    while (n > 0) {
        n /= 10;
        ++width;
    }
    return (width == 0) ? 1 : width;
}

#endif // ZOGRASCOPE_UTILS_NUMS_HPP_
