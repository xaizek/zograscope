#ifndef TREEBUILDER_HPP__
#define TREEBUILDER_HPP__

#include <cstddef>

#include <deque>
#include <utility>
#include <vector>

struct Text
{
    std::size_t from, len;
};

struct PNode
{
    Text value;
    std::vector<PNode *> children;
};

class TreeBuilder
{
public:
    PNode * addNode(PNode *node)
    {
        return node;
    }

    PNode * addNode(Text value = {})
    {
        nodes.push_back({ value });
        return &nodes.back();
    }

    PNode * addNode(std::vector<PNode *> children)
    {
        nodes.push_back({ {}, std::move(children) });
        return &nodes.back();
    }

    PNode * append(PNode *node, PNode *child)
    {
        node->children.push_back(child);
        return node;
    }

    void setRoot(PNode *node)
    {
        root = node;
    }

    PNode * getRoot()
    {
        return root;
    }

private:
    std::deque<PNode> nodes;
    PNode *root;
};

#endif // TREEBUILDER_HPP__
