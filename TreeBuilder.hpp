#ifndef TREEBUILDER_HPP__
#define TREEBUILDER_HPP__

#include <cstddef>

#include <deque>
#include <utility>
#include <vector>

#include "types.hpp"

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
    std::size_t postponedFrom, postponedTo;
    int token;
};

struct PNode
{
    PNode()
    {
    }
    PNode(std::vector<PNode *> children) : children(std::move(children))
    {
        for (PNode *&child : this->children) {
            child = contract(child);
        }
    }
    PNode(Text value, const Location &loc)
        : value(value), line(loc.first_line), col(loc.first_column)
    {
    }

    bool empty() const
    {
        return value.from == 0U && value.len == 0U;
    }

    Text value = { 0, 0 };
    std::vector<PNode *> children;
    int line = 0, col = 0;

private:
    static PNode * contract(PNode *node)
    {
        if (node->empty() && node->children.size() == 1U &&
            node->children.front()->empty()) {
            return contract(node->children.front());
        }
        return node;
    }
};

class TreeBuilder
{
    struct Postponed
    {
        Text value;
        Location loc;
    };

public:
    PNode * addNode(PNode *node, const Location &loc)
    {
        return addNode({ node });
    }

    PNode * addNode()
    {
        nodes.emplace_back();
        return &nodes.back();
    }

    PNode * addNode(Text value, const Location &loc)
    {
        if (value.postponedFrom != value.postponedTo) {
            std::vector<PNode *> children;
            children.reserve(value.postponedTo - value.postponedFrom);
            for (std::size_t i = value.postponedFrom; i < value.postponedTo;
                 ++i) {
                children.push_back(addNode(postponed[i].value,
                                           postponed[i].loc));
            }
            nodes.emplace_back(value, loc);
            children.push_back(&nodes.back());
            nodes.emplace_back(std::move(children));
            return &nodes.back();
        }

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

    PNode * prepend(PNode *node, PNode *child)
    {
        node->children.insert(node->children.cbegin(), child);
        return node;
    }

    PNode * append(PNode *node, const std::initializer_list<PNode *> &c)
    {
        node->children.insert(node->children.cend(), c);
        return node;
    }

    PNode * prepend(PNode *node, const std::initializer_list<PNode *> &c)
    {
        node->children.insert(node->children.cbegin(), c);
        return node;
    }

    void addPostponed(Text value, const Location &loc)
    {
        postponed.push_back({ value, loc });
        ++newPostponed;
    }

    void markWithPostponed(Text &value)
    {
        value.postponedFrom = postponed.size() - newPostponed;
        value.postponedTo = postponed.size();
        newPostponed = 0;
    }

    void finish(bool failed)
    {
        if (failed) {
            this->failed = failed;
            return;
        }

        std::vector<PNode *> children;
        children.reserve(newPostponed);
        for (std::size_t i = postponed.size() - newPostponed;
             i < postponed.size(); ++i) {
            children.push_back(addNode(postponed[i].value, postponed[i].loc));
        }

        root->children.insert(root->children.cend(),
                              children.cbegin(), children.cend());
    }

    void setRoot(PNode *node)
    {
        root = node;
    }

    PNode * getRoot()
    {
        return root;
    }

    bool hasFailed() const
    {
        return failed;
    }

private:
    std::deque<PNode> nodes;
    PNode *root = nullptr;
    std::vector<Postponed> postponed;
    int newPostponed = 0;
    bool failed = false;
};

#endif // TREEBUILDER_HPP__
