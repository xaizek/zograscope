#ifndef TREEBUILDER_HPP__
#define TREEBUILDER_HPP__

#include <cstddef>

#include <deque>
#include <utility>
#include <vector>

struct Location
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};

struct Text
{
    std::size_t from, len;
};

struct PNode
{
    PNode()
    {
    }
    PNode(std::vector<PNode *> children) : children(std::move(children))
    {
    }
    PNode(Text value, const Location &loc)
        : value(value), line(loc.first_line), col(loc.first_column)
    {
    }

    Text value = { 0, 0 };
    std::vector<PNode *> children;
    int line = 0, col = 0;
};

class TreeBuilder
{
public:
    PNode * addNode(PNode *node, const Location &loc)
    {
        return node;
    }

    PNode * addNode()
    {
        nodes.emplace_back();
        return &nodes.back();
    }

    PNode * addNode(Text value, const Location &loc)
    {
        nodes.emplace_back(value, loc);
        return &nodes.back();
    }

    PNode * addNode(std::vector<PNode *> children)
    {
        nodes.emplace_back(std::move(children));
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
    PNode *root = nullptr;
};

#endif // TREEBUILDER_HPP__
