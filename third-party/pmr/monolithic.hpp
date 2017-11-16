/* Copyright 2017 xaizek.
 *
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PMR__MONOLITHIC_HPP__
#define PMR__MONOLITHIC_HPP__

#include "pmr_vector.hpp"
#include "polymorphic_allocator.hpp"

namespace cpp17 {

using namespace std;

namespace pmr {

class monolithic final : public memory_resource
{
    enum {
        blockSize = 64*1024,
        alignment = alignof(std::max_align_t),
    };

public:
    explicit monolithic(memory_resource *parent = get_default_resource());
    virtual ~monolithic() override;

protected:
    virtual void * do_allocate(size_t bytes, size_t alignment) override;
    virtual void do_deallocate(void *p, size_t bytes,
                               size_t alignment) override;
    virtual bool do_is_equal(const memory_resource &other)
        const noexcept override;

private:
    struct Block {
        size_t size;
        byte *start;
        byte *next;

        byte * aligned(size_t alignment) const;
        byte * allocate(size_t n, size_t alignment);
    };

    memory_resource *parent;
    vector<Block> blocks;
};

inline byte *
monolithic::Block::aligned(size_t alignment) const
{
    const size_t mod = reinterpret_cast<uintptr_t>(next)&(alignment - 1U);
    return (mod == 0U ? next : next + alignment - mod);
}

inline byte *
monolithic::Block::allocate(size_t n, size_t alignment)
{
    byte *ret = aligned(alignment);
    if (static_cast<size_t>(start + size - ret) < n) {
        return nullptr;
    }

    next = ret + n;
    return ret;
}

inline
monolithic::monolithic(memory_resource *parent) : parent(parent), blocks(parent)
{
}

inline
monolithic::~monolithic()
{
    for (Block &alloc_rec : blocks) {
        parent->deallocate(alloc_rec.start, alloc_rec.size, alignment);
    }
}

inline void *
monolithic::do_allocate(size_t bytes, size_t align)
{
    void *ret;
    if (blocks.empty() || !(ret = blocks.back().allocate(bytes, align))) {
        const size_t size = max(static_cast<std::size_t>(blockSize), bytes);

        byte *r = static_cast<byte *>(parent->allocate(size, alignment));
        blocks.push_back(Block{size, r, r});
        ret = blocks.back().allocate(bytes, align);
    }

    return ret;
}

inline void
monolithic::do_deallocate(void */*p*/, size_t /*bytes*/, size_t /*alignment*/)
{
    // Do nothing.
}

inline bool
monolithic::do_is_equal(const memory_resource &other) const noexcept
{
    return this == &other;
}

} // close namespace pmr

} // close namespace cpp17

#endif // PMR__MONOLITHIC_HPP__
