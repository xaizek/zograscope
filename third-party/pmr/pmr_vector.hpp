/* Copyright 2017 Pablo Halpern.
 *
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PMR_PMR_VECTOR_HPP_
#define PMR_PMR_VECTOR_HPP_

#include <vector>

#include "polymorphic_allocator.hpp"

namespace cpp17 {
namespace pmr {

    // C++17 vector container that uses a polymorphic allocator.
    template <class Tp>
    using vector = std::vector<Tp, polymorphic_allocator<Tp>>;

}
}

#endif // PMR_PMR_VECTOR_HPP_
