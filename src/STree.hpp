#ifndef STREE_HPP__
#define STREE_HPP__

#include <string>

#include "pmr/pmr_vector.hpp"

#include "utils/Pool.hpp"
#include "TreeBuilder.hpp"

namespace cpp17 {
    namespace pmr {
        class monolithic;
    }
}

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
          bool dumpWhole, bool dumpUnclear,
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

#endif // STREE_HPP__
