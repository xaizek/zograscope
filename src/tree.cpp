#include "tree.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "TreeBuilder.hpp"
#include "types.hpp"

void
print(const Node &node, int lvl)
{
    // if (node.satellite) {
    //     return;
    // }

    std::string prefix = (lvl > 0) ? "`---" : "";

    std::string suffix;
    switch (node.state) {
        case State::Unchanged: break;
        case State::Deleted:   suffix = " (deleted)";  break;
        case State::Inserted:  suffix = " (inserted)"; break;
        case State::Updated:   suffix = " (updated with " + node.relative->label + "@" + std::to_string(node.relative->poID) + ")";  break;
    }

    std::cout << std::setw(4*lvl) << prefix << node.label
              << '[' << node.poID << ']'
              << '(' << node.line << ';' << node.col << ')'
              << (node.satellite ? "{S}" : "")
              << suffix
              << '<' << static_cast<int>(node.type) << '>'
              << '\n';
    for (const Node &child : node.children) {
        print(child, lvl + 1);
    }
}

static std::string
materializeLabel(const std::string &contents, const PNode *node)
{
    // lexer also has such variable and they need to be synchronized (actually
    // we should pass this to lexer)
    enum { tabWidth = 4 };

    std::string label;
    label.reserve(node->value.len);

    int col = node->col;
    for (std::size_t i = node->value.from;
         i < node->value.from + node->value.len; ++i) {
        switch (const char c = contents[i]) {
            int width;

            case '\n':
                col = 1;
                label += '\n';
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                label.append(width, ' ');
                col += width;
                break;

            default:
                ++col;
                label += c;
                break;
        }
    }

    return label;
}

Node
materializeTree(const std::string &contents, const PNode *node)
{
    Node n;
    n.label = materializeLabel(contents, node);
    n.line = node->line;
    n.col = node->col;
    n.type = tokenToType(node->value.token);

    n.children.reserve(node->children.size());
    for (const PNode *child : node->children) {
        n.children.push_back(materializeTree(contents, child));
    }

    return n;
}

static void
postOrder(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    for (Node &child : node.children) {
        child.relative = &node;
        postOrder(child, v);
    }
    node.poID = v.size();
    v.push_back(&node);
}

std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    root.relative = &root;
    postOrder(root, v);
    return v;
}
