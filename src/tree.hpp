#ifndef TREE_HPP__
#define TREE_HPP__

#include <string>
#include <vector>

#include "types.hpp"

class PNode;

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
    std::vector<Node> children;
    int poID = -1; // post-order ID
    State state = State::Unchanged;
    int line = 0;
    int col = 0;
    Node *relative = nullptr;
    bool satellite = false;
    Type type = Type::Virtual;
};

void print(const Node &node, int lvl = 0);

Node materializeTree(const std::string &contents, const PNode *node);

#endif // TREE_HPP__
