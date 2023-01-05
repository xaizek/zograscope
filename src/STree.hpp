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

#ifndef ZOGRASCOPE_STREE_HPP_
#define ZOGRASCOPE_STREE_HPP_

#include <string>

#include "pmr/pmr_vector.hpp"

#include "utils/Pool.hpp"
#include "TreeBuilder.hpp"

namespace cpp17 {
    namespace pmr {
        class monolithic;
    }
}

class Language;

struct SNode
{
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

    SNode(PNode *value, allocator_type al = {})
        : value(value), children(al)
    {
    }

    PNode *value;
    cpp17::pmr::vector<SNode *> children;
};

class STree
{
public:
    STree(TreeBuilder &&ptree, const std::string &contents,
          bool dumpWhole, bool dumpUnclear, Language &lang,
          cpp17::pmr::monolithic &mr);

    STree(const STree &rhs) = delete;
    STree(STree &&rhs) = delete;
    STree & operator=(const STree &rhs) = delete;
    STree & operator=(STree &&rhs) = default;

public:
    SNode * getRoot() { return root; }

private:
    TreeBuilder ptree;
    Pool<SNode> pool;
    SNode *root;
};

#endif // ZOGRASCOPE_STREE_HPP_
