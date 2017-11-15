#ifndef STREE_HPP__
#define STREE_HPP__

#include <deque>
#include <string>
#include <vector>

#include "TreeBuilder.hpp"

struct SNode
{
    SNode(PNode *value) : value(value)
    {
    }

    PNode *value;
    std::vector<SNode *> children;
};

class STree
{
public:
    STree(TreeBuilder &&ptree, const std::string &contents,
          bool dumpWhole = false, bool dumpUnclear = false);

    STree(const STree &rhs) = delete;
    STree(STree &&rhs) = delete;
    STree & operator=(const STree &rhs) = delete;
    STree & operator=(STree &&rhs) = default;

public:
    SNode * getRoot() { return root; }

private:
    TreeBuilder ptree;
    std::deque<SNode> snodes;
    SNode *root;
};

#endif // STREE_HPP__
