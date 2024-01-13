// Copyright (C) 2024 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_UTILS_ITERATORS_HPP_
#define ZOGRASCOPE_UTILS_ITERATORS_HPP_

#include <utility>

namespace guts
{

template <typename I>
struct ItPair
{
    I from;
    I to;

    I & begin() { return from; }
    I & end()   { return to; }
};

}

template <typename I, typename... Args>
guts::ItPair<I>
rangeFrom(Args &&...args)
{
    return guts::ItPair<I>{ I(std::forward<Args>(args)...), I() };
}

#endif // ZOGRASCOPE_UTILS_ITERATORS_HPP_
