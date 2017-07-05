#ifndef TREE_HPP__
#define TREE_HPP__

#include <string>
#include <vector>

#include "TreeBuilder.hpp"
#include "types.hpp"

enum class State
{
    Unchanged,
    Deleted,
    Inserted,
    Updated
};

struct Node
{
    std::string label;
    std::vector<Node *> children;
    int poID = -1; // post-order ID
    State state = State::Unchanged;
    int line = 0;
    int col = 0;
    Node *relative = nullptr;
    bool satellite = false;
    Type type = Type::Virtual;
    SType stype = SType::None;
    Node *next = nullptr;
};

void print(const Node &node, int lvl = 0);
class Tree
{
public:
    Tree() = default;
    Tree(const std::string &contents, const PNode *node);
    Tree(const std::string &contents, const SNode *node);

public:
    Node * getRoot()
    {
        return root;
    }

    const Node * getRoot() const
    {
        return root;
    }

    Node & makeNode()
    {
        nodes.emplace_back();
        return nodes.back();
    }

private:
    std::deque<Node> nodes;
    Node *root = nullptr;
};


std::vector<Node *> postOrder(Node &root);

void reduceTreesFine(Node *&T1, Node *&T2);

void reduceTreesCoarse(Node *T1, Node *T2);

#endif // TREE_HPP__
