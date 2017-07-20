#ifndef TREEBUILDER_HPP__
#define TREEBUILDER_HPP__

#include <boost/range/adaptor/reversed.hpp>

#include <cstddef>

#include <deque>
#include <iosfwd>
#include <utility>
#include <vector>

enum class SType
{
    None,
    TranslationUnit,
    Declaration,
    Declarations,
    FunctionDeclaration,
    FunctionDefinition,
    Comment,
    Directive,
    Macro,
    CompoundStatement,
    Separator,
    Statements,
    Statement,
    IfStmt,
    IfCond,
    IfElse,
    WhileStmt,
    WhileCond,
    ForStmt,
    ForHead,
    Expression,
    Declarator,
    Initializer,
    InitializerList,
    Specifiers,
    WithInitializer,
    InitializerElement,
};

std::ostream & operator<<(std::ostream &os, SType stype);

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
    PNode(std::vector<PNode *> children, SType stype = SType::None)
        : children(std::move(children)), stype(stype)
    {
        for (PNode *&child : this->children) {
            child = contract(child);
        }
    }
    PNode(Text value, const Location &loc, SType stype = SType::None,
          bool postponed = false)
        : value(value), line(loc.first_line), col(loc.first_column),
          postponed(postponed), stype(stype)
    {
    }

    bool empty() const
    {
        return value.from == 0U && value.len == 0U && stype == SType::None;
    }

    Text value = { };
    std::vector<PNode *> children;
    int line = 0, col = 0;
    bool postponed = false;
    SType stype = SType::None;
    int movedChildren = 0;

private:
    // TODO: try contracting in TreeBuilder as well (this way we won't even
    //       create extra node)
    static PNode * contract(PNode *node)
    {
        if (node->empty() && node->children.size() == 1U &&
            node->children.front()->empty()) {
            // TODO: we could reuse contracted nodes to save some memory
            return contract(node->children.front());
        }
        return node;
    }
};

struct SNode
{
    PNode *value;
    std::vector<SNode *> children;
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
    PNode * addNode(PNode *node, const Location &, SType stype = SType::None)
    {
        return addNode({ node }, stype);
    }

    PNode * addNode()
    {
        nodes.emplace_back();
        return &nodes.back();
    }

    PNode * addNode(Text value, const Location &loc, SType stype = SType::None)
    {
        return addNode(value, loc, value.token, stype);
    }

    PNode * addNode(Text value, const Location &loc, int token,
                    SType stype = SType::None);

    PNode * addNode(std::vector<PNode *> children, SType stype = SType::None);

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

    SNode * makeSTree(const std::string &contents, bool dumpWhole = false,
                      bool dumpUnclear = false);

private:
    void movePostponed(PNode *&node, std::vector<PNode *> &nodes,
                       std::vector<PNode *>::const_iterator insertPos);

private:
    std::deque<PNode> nodes;
    std::deque<SNode> snodes;
    PNode *root = nullptr;
    std::vector<Postponed> postponed;
    int newPostponed = 0;
    bool failed = false;
};

#endif // TREEBUILDER_HPP__
