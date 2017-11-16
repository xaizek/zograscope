#ifndef POOL_HPP__
#define POOL_HPP__

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

#endif // POOL_HPP__
