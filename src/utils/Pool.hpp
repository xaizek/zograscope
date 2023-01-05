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

#ifndef ZOGRASCOPE_UTILS_POOL_HPP_
#define ZOGRASCOPE_UTILS_POOL_HPP_

#include <utility>

#include "pmr/polymorphic_allocator.hpp"

// A fabric of objects of specified type that does NOT destruct them.  Must be
// used only with types which don't do anything useful in destructors due to
// allocator cleaning everything up for them.
template <typename T>
class Pool
{
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

public:
    // Allocator must be specified, this class should not be used with a default
    // allocator by mistake.
    explicit Pool(allocator_type al) : alloc(al)
    {
    }

    // Forwards everything to the constructor of the object.
    template <typename... Args>
    T * make(Args &&...args)
    {
        auto data = static_cast<T *>(alloc.resource()->allocate(sizeof(T),
                                                                alignof(T)));
        try {
            alloc.construct(data, std::forward<Args>(args)...);
        } catch (...) {
            alloc.resource()->deallocate(data, sizeof(T), alignof(T));
            throw;
        }

        return data;
    }

private:
    allocator_type alloc; // Allocator used for objects.
};

#endif // ZOGRASCOPE_UTILS_POOL_HPP_
