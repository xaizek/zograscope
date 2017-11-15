/* Copyright 2012 Pablo Halpern.
 *
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include "polymorphic_allocator.hpp"

namespace cpp17 {
namespace pmr {

atomic<memory_resource *> memory_resource::s_default_resource(nullptr);

new_delete_resource *
new_delete_resource_singleton()
{
    static new_delete_resource singleton;
    return &singleton;
}

}
}
