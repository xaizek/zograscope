#ifndef STREE_HPP__
#define STREE_HPP__

#include <string>

#include "pmr/pmr_vector.hpp"

#include "Pool.hpp"
#include "TreeBuilder.hpp"

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
    using allocator_type = cpp17::pmr::polymorphic_allocator<cpp17::byte>;

public:
    STree(TreeBuilder &&ptree, const std::string &contents,
          bool dumpWhole = false, bool dumpUnclear = false,
          allocator_type al = {});

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

#endif // STREE_HPP__
