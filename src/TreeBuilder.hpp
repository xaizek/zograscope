#ifndef TREEBUILDER_HPP__
#define TREEBUILDER_HPP__

#include <boost/range/adaptor/reversed.hpp>

#include <cstddef>

#include <deque>
#include <utility>
#include <vector>

enum class SType;

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
    PNode(std::vector<PNode *> children, SType stype = {})
        : children(std::move(children)), stype(stype)
    {
        for (PNode *&child : this->children) {
            child = contract(child);
        }
    }
    PNode(Text value, const Location &loc, SType stype = {},
          bool postponed = false)
        : value(value), line(loc.first_line), col(loc.first_column),
          postponed(postponed), stype(stype)
    {
    }

    PNode(const PNode &) = delete;
    PNode(PNode &&) = delete;
    PNode & operator=(const PNode &) = delete;
    PNode & operator=(PNode &&) = delete;

    bool empty() const
    {
        return value.from == 0U && value.len == 0U && stype == SType{};
    }

    Text value = { 0U, 0U, 0U, 0U, 0 };
    std::vector<PNode *> children;
    int line = 0, col = 0;
    bool postponed = false;
    SType stype = {};
    int movedChildren = 0;

    static PNode * contract(PNode *node);
};

class TreeBuilder
{
    struct Postponed
    {
        Text value;
        Location loc;
        SType stype;
    };

public:
    TreeBuilder() = default;
    TreeBuilder(const TreeBuilder &rhs) = delete;
    TreeBuilder(TreeBuilder &&rhs) = default;
    TreeBuilder & operator=(const TreeBuilder &rhs) = delete;
    TreeBuilder & operator=(TreeBuilder &&rhs) = delete;

public:
    PNode * addNode(PNode *node, const Location &)
    {
        return node;
    }

    PNode * addNode(PNode *node, const Location &, SType stype)
    {
        return addNode({ node }, stype);
    }

    PNode * addNode()
    {
        nodes.emplace_back();
        return &nodes.back();
    }

    PNode * addNode(Text value, const Location &loc, SType stype = {})
    {
        return addNode(value, loc, value.token, stype);
    }

    PNode * addNode(Text value, const Location &loc, int token,
                    SType stype = {});

    PNode * addNode(std::vector<PNode *> children, SType stype = {});

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

    void addPostponed(Text value, const Location &loc, SType stype)
    {
        postponed.push_back({ value, loc, stype });
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
