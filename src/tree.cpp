#include "tree.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/functional/hash.hpp>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "STree.hpp"
#include "TreeBuilder.hpp"
#include "trees.hpp"
#include "types.hpp"
#include "utils.hpp"

static void printNode(std::ostream &os, const Node *node);
static std::string materializePTree(const std::string &contents,
                                    const PNode *node);
static bool areIdentical(const Node *l, const Node *r);

void
print(const Node &node)
{
    trees::print(std::cout, &node, &printNode);
}

static void
printNode(std::ostream &os, const Node *node)
{
    // if (node->satellite) {
    //     return;
    // }

    auto l = [](const std::string &s) {
        return boost::replace_all_copy(s, "\n", "<NL>");
    };

    std::string suffix;
    switch (node->state) {
        case State::Unchanged: break;
        case State::Deleted:   suffix = " (deleted)";  break;
        case State::Inserted:  suffix = " (inserted)"; break;
        case State::Updated:   suffix = " (updated with " + l(node->relative->label) + "@" + std::to_string(node->relative->poID) + ")";  break;
    }

    if (node->relative != nullptr && node->state != State::Updated) {
        suffix += " (relative: " + l(node->relative->label) + "@" + std::to_string(node->relative->poID) + ")";
    }

    os << l(node->label)
       << '[' << node->poID << ']'
       << '(' << node->line << ';' << node->col << ')'
       << (node->satellite ? "{S}" : "")
       << suffix
       << ", Type::" << node->type
       << ", SType::" << node->stype
       << '\n';
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

static Node *
materializeNode(Tree &tree, const std::string &contents, const PNode *node)
{
    Node &n = tree.makeNode();
    n.label = materializeLabel(contents, node);
    n.line = node->line;
    n.col = node->col;
    n.type = tokenToType(node->value.token);
    // This is for debugging purposes on dumping tree.
    n.stype = node->stype;

    n.children.reserve(node->children.size());
    for (const PNode *child : node->children) {
        n.children.push_back(materializeNode(tree, contents, child));
    }

    return &n;
}

Tree::Tree(const std::string &contents, const PNode *node)
{
    root = materializeNode(*this, contents, node);
}

static Node *
materializeNode(Tree &tree, const std::string &contents, const SNode *node)
{
    std::function<const PNode *(const PNode *)> lml = [&](const PNode *node) {
        return node->children.empty()
             ? node
             : lml(node->children.front());
    };

    std::function<Node *(const SNode *)> visit = [&](const SNode *node) {
        Node &n = tree.makeNode();
        n.stype = node->value->stype;
        n.satellite = (n.stype == SType::Separator);

        if (node->children.empty()) {
            const PNode *leaf = lml(node->value);

            n.label = materializePTree(contents, node->value);
            n.line = leaf->line;
            n.col = leaf->col;
            n.next = materializeNode(tree, contents, node->value);
            return &n;
        }

        n.children.reserve(node->children.size());
        for (SNode *child : node->children) {
            n.children.emplace_back(visit(child));
        }

        auto valueChild = std::find_if(node->children.begin(),
                                       node->children.end(),
                                       [](SNode *n) {
                                           return n->value->stype ==
                                                  SType::FunctionDeclaration
                                               || n->value->stype ==
                                                  SType::IfExpr;
                                       });
        if (valueChild != node->children.end()) {
            n.label = materializePTree(contents, (*valueChild)->value);
            n.valueChild = valueChild - node->children.begin();
        } else {
            n.valueChild = -1;
        }

        return &n;
    };

    return visit(node);
}

Tree::Tree(const std::string &contents, const SNode *node)
{
    root = materializeNode(*this, contents, node);
}

static std::string
materializePTree(const std::string &contents, const PNode *node)
{
    std::string out;

    std::function<void(const PNode *)> visit = [&](const PNode *node) {
        if (node->line != 0 && node->col != 0) {
            out += materializeLabel(contents, node);
        }

        for (const PNode *child : node->children) {
            visit(child);
        }
    };
    visit(node);

    return out;
}

static void
postOrder(Node &node, std::vector<Node *> &v)
{
    if (node.satellite) {
        return;
    }

    for (Node *child : node.children) {
        child->parent = &node;
        postOrder(*child, v);
    }
    node.poID = v.size();
    v.push_back(&node);
}

std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    root.parent = &root;
    postOrder(root, v);
    return v;
}

static std::size_t
hashNode(const Node *node)
{
    const std::size_t hash = std::hash<std::string>()(node->label);
    return std::accumulate(node->children.cbegin(), node->children.cend(), hash,
                           [](std::size_t h, const Node *child) {
                               boost::hash_combine(h, hashNode(child));
                               return h;
                           });
}

static std::vector<std::size_t>
hashChildren(const Node &node)
{
    std::vector<std::size_t> hashes;
    hashes.reserve(node.children.size());
    for (const Node *child : node.children) {
        hashes.push_back(hashNode(child));
    }
    return hashes;
}

void
reduceTreesCoarse(Node *T1, Node *T2)
{
    const std::vector<std::size_t> children1 = hashChildren(*T1);
    const std::vector<std::size_t> children2 = hashChildren(*T2);

    for (unsigned int i = 0U, n = children1.size(); i < n; ++i) {
        const std::size_t hash1 = children1[i];
        for (unsigned int j = 0U, n = children2.size(); j < n; ++j) {
            if (T2->children[j]->satellite) {
                continue;
            }

            const std::size_t hash2 = children2[j];
            if (hash1 == hash2) {
                T1->children[i]->relative = T2->children[j];
                T1->children[i]->satellite = true;
                T2->children[j]->relative = T1->children[i];
                T2->children[j]->satellite = true;
                break;
            }
        }
    }
}

static bool
areIdentical(const Node *l, const Node *r)
{
    if (l->label != r->label || l->children.size() != r->children.size()) {
        return false;
    }

    for (unsigned int i = 0; i < l->children.size(); ++i) {
        if (!areIdentical(l->children[i], r->children[i])) {
            return false;
        }
    }

    return true;
}

std::string
printSubTree(const Node &root)
{
    std::string out;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.line != 0 && node.col != 0) {
            const std::vector<std::string> lines = split(node.label, '\n');
            out += node.label;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };
    visit(root);

    return out;
}
