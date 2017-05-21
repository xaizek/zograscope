#ifndef TREE_EDIT_DISTANCE_HPP__
#define TREE_EDIT_DISTANCE_HPP__

#include <string>
#include <vector>

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
    std::vector<Node> children;
    int poID; // post-order ID
    State state;
    int line;
    int col;
    Node *relative;
    bool satellite;
    Type type;
};

void printTree(const std::string &name, Node &root);

void print(const Node &node, int lvl = 0);

int ted(Node &T1, Node &T2);

#endif // TREE_EDIT_DISTANCE_HPP__
