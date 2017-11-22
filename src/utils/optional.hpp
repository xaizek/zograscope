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

#ifndef UTILS__OPTIONAL_HPP__
#define UTILS__OPTIONAL_HPP__

#include <utility>

#include <boost/optional/optional_fwd.hpp>

template <typename T>
class MoveOnCopy
{
public:
    MoveOnCopy(T &&movable) : movable(std::move(movable))
    {
    }

    MoveOnCopy(MoveOnCopy &rhs) : movable(std::move(rhs.movable))
    {
    }

public:
    operator T&&() const
    {
        return std::move(movable);
    }

private:
    mutable T movable;
};

template <typename T>
using optional_t = boost::optional<MoveOnCopy<T>>;

#endif // UTILS__OPTIONAL_HPP__
