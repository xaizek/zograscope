/* Copyright 2017 xaizek.
 *
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PMR__PMR_DEQUE_HPP__
#define PMR__PMR_DEQUE_HPP__

#include <deque>

#include "polymorphic_allocator.hpp"

namespace cpp17 {
namespace pmr {

    // C++17 deque container that uses a polymorphic allocator.
    template <class Tp>
    using deque = std::deque<Tp, polymorphic_allocator<Tp>>;

}
}

#endif // PMR__PMR_DEQUE_HPP__
