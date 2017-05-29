#ifndef TREEBUILDER_HPP__
#define TREEBUILDER_HPP__

#include <boost/range/adaptor/reversed.hpp>

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
    PNode(Text value, const Location &loc, bool postponed = false)
        : value(value), line(loc.first_line), col(loc.first_column),
          postponed(postponed)
    {
    }

    bool empty() const
    {
        return value.from == 0U && value.len == 0U;
    }

    Text value = { };
    std::vector<PNode *> children;
    int line = 0, col = 0;
    bool postponed = false;

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
    PNode * addNode(PNode *node, const Location &)
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
        return addNode(value, loc, value.token);
    }

    PNode * addNode(Text value, const Location &loc, int token);

    PNode * addNode(std::vector<PNode *> children)
    {
        movePostponed(children[0], children, children.cbegin());
        nodes.emplace_back(std::move(children));
        return &nodes.back();
    }

    PNode * append(PNode *node, PNode *child)
    {
        movePostponed(child, node->children, node->children.cend());
        node->children.push_back(child);
        return node;
    }

    PNode * prepend(PNode *node, PNode *child)
    {
        const auto sizeWas = node->children.size();
        movePostponed(child, node->children, node->children.cbegin());
        const auto sizeChange = node->children.size() - sizeWas;
        node->children.insert(node->children.cbegin() + sizeChange, child);
        return node;
    }

    PNode * append(PNode *node, const std::initializer_list<PNode *> &children)
    {
        for (PNode *child : children) {
            append(node, child);
        }
        return node;
    }

    PNode * prepend(PNode *node, const std::initializer_list<PNode *> &children)
    {
        for (PNode *child : boost::adaptors::reverse(children)) {
            prepend(node, child);
        }
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

    void finish(bool failed);

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
    void movePostponed(PNode *&node, std::vector<PNode *> &nodes,
                       std::vector<PNode *>::const_iterator insertPos);

private:
    std::deque<PNode> nodes;
    PNode *root = nullptr;
    std::vector<Postponed> postponed;
    int newPostponed = 0;
    bool failed = false;
};

#endif // TREEBUILDER_HPP__
