#include "STree.hpp"

#include <deque>
#include <iostream>
#include <utility>
#include <vector>

#include "Pool.hpp"
#include "decoration.hpp"
#include "stypes.hpp"
#include "trees.hpp"

static void print(const PNode *node, const std::string &contents);
static PNode * findSNode(PNode *node);
static SNode * makeSNode(Pool<SNode> &snodes, const std::string &contents,
                         PNode *pnode, bool dumpUnclear);

STree::STree(TreeBuilder &&ptree, const std::string &contents, bool dumpWhole,
             bool dumpUnclear, allocator_type al)
    : ptree(std::move(ptree)), pool(al)
{
    PNode *proot = ptree.getRoot();

    if (dumpWhole) {
        print(proot, contents);
    }

    PNode *rootNode = findSNode(proot);
    if (rootNode == nullptr) {
        root = pool.make(proot);
        return;
    }

    root = makeSNode(pool, contents, rootNode, dumpUnclear);
}

static void
print(const PNode *node, const std::string &contents)
{
    using namespace decor;
    using namespace decor::literals;

    Decoration labelHi = 78_fg + bold;
    Decoration stypeHi = 222_fg;

    trees::print(std::cout, node, [&](std::ostream &os, const PNode *node) {
        os << (labelHi << '`'
                       << contents.substr(node->value.from, node->value.len)
                       << '`')
           << ", "
           << (stypeHi << "SType::" << node->stype)
           << '\n';
    });
}

static PNode *
findSNode(PNode *node)
{
    if (node->stype != SType{}) {
        return node;
    }

    return node->children.size() == 1U
         ? findSNode(node->children.front())
         : nullptr;
}

static SNode *
makeSNode(Pool<SNode> &pool, const std::string &contents, PNode *pnode,
          bool dumpUnclear)
{
    SNode *snode = pool.make(pnode);

    auto isSNode = [](PNode *child) {
        return (findSNode(child) != nullptr);
    };
    // If none of the children is SNode, then current node is a leaf SNode.
    if (std::none_of(pnode->children.begin(), pnode->children.end(), isSNode)) {
        return snode;
    }

    cpp17::pmr::vector<SNode *> &c = snode->children;
    c.reserve(pnode->children.size());
    for (PNode *child : pnode->children) {
        if (PNode *schild = findSNode(child)) {
            c.push_back(makeSNode(pool, contents, schild, dumpUnclear));
        } else {
            if (dumpUnclear) {
                print(child, contents);
            }
            c.push_back(pool.make(pnode->children[c.size()]));
        }
    }
    return snode;
}
