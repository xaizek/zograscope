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
#include "decoration.hpp"
#include "trees.hpp"
#include "types.hpp"
#include "utils.hpp"

static void printTree(const Node *node, std::vector<bool> &trace, int depth);
static void printNode(std::ostream &os, const Node *node);
static std::string materializePTree(const std::string &contents,
                                    const PNode *node);
static bool areIdentical(const Node *l, const Node *r);

void
print(const Node &node)
{
    std::vector<bool> trace;
    printTree(&node, trace, 0);
}

static void
printTree(const Node *node, std::vector<bool> &trace, int depth)
{
    using namespace decor;
    using namespace decor::literals;

    Decoration sepHi = 246_fg;
    Decoration depthHi = 250_fg;

    std::cout << sepHi;

    std::cout << (trace.empty() ? "--- " : "    ");

    for (unsigned int i = 0U, n = trace.size(); i < n; ++i) {
        bool last = (i == n - 1U);
        if (trace[i]) {
            std::cout << (last ? "`-- " : "    ");
        } else {
            std::cout << (last ? "|-- " : "|   ");
        }
    }

    std::cout << def;

    std::cout << (depthHi << depth) << (sepHi << " | ");
    printNode(std::cout, node);

    trace.push_back(false);
    for (unsigned int i = 0U, n = node->children.size(); i < n; ++i) {
        Node *child = node->children[i];

        trace.back() = (i == n - 1U);
        printTree(child, trace, depth);

        if (child->next != nullptr && !child->next->last) {
            trace.push_back(true);
            printTree(child->next, trace, depth + 1);
            trace.pop_back();
        }
    }
    trace.pop_back();
}

static void
printNode(std::ostream &os, const Node *node)
{
    using namespace decor;
    using namespace decor::literals;

    // if (node->satellite) {
    //     return;
    // }

    Decoration labelHi = 78_fg + bold;
    Decoration relLabelHi = 78_fg;
    Decoration idHi = bold;
    Decoration movedHi = 33_fg + inv + bold + 231_bg;
    Decoration insHi = 82_fg + inv + bold;
    Decoration updHi = 226_fg + inv + bold;
    Decoration delHi = 160_fg + inv + bold + 231_bg;
    Decoration relHi = 226_fg + bold;
    Decoration typeHi = 51_fg;
    Decoration stypeHi = 222_fg;

    auto l = [](const std::string &s) {
        return '`' + boost::replace_all_copy(s, "\n", "<NL>") + '`';
    };

    if (node->moved) {
        os << (movedHi << '!');
    }

    switch (node->state) {
        case State::Unchanged: break;
        case State::Deleted:  os << (delHi << '-'); break;
        case State::Inserted: os << (insHi << '+'); break;
        case State::Updated:  os << (updHi << '~'); break;
    }

    os << (labelHi << l(node->label))
       << (idHi << " #" << node->poID);

    os << (node->satellite ? ", Satellite" : "") << ", "
       << (typeHi << "Type::" << node->type) << ", "
       << (stypeHi << "SType::" << node->stype);

    if (node->relative != nullptr) {
        os << (relHi << " -> ") << (relLabelHi << l(node->relative->label))
           << (idHi << " #" << node->relative->poID);
    }

    os << '\n';
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
    const Type type = tokenToType(node->value.token);

    if (type == Type::Virtual && node->children.size() == 1U) {
        return materializeNode(tree, contents, node->children[0]);
    }

    Node &n = tree.makeNode();
    n.label = materializeLabel(contents, node);
    n.line = node->line;
    n.col = node->col;
    n.type = type;
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

static bool
shouldSplice(SType parent, SType child)
{
    if (parent == SType::IfThen || parent == SType::IfElse) {
        if (child == SType::CompoundStatement) {
            return true;
        }
    }

    if (parent == SType::IfStmt) {
        if (child == SType::IfThen) {
            return true;
        }
    }

    return false;
}

static bool
isValueSNode(const SNode *n)
{
    return n->value->stype == SType::FunctionDeclaration
        || n->value->stype == SType::IfExpr;
}

static bool
isLayerBreak(SType stype)
{
    switch (stype) {
        case SType::FunctionDefinition:
        case SType::InitializerElement:
        case SType::InitializerList:
        case SType::Initializer:
        case SType::Declaration:
        case SType::ExprStatement:
            return true;

        default:
            return false;
    };
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
            n.next->last = true;
            n.type = n.next->type;
            return &n;
        }

        n.children.reserve(node->children.size());
        for (SNode *child : node->children) {
            Node *newChild = visit(child);

            if (shouldSplice(node->value->stype, child->value->stype)) {
                n.children.insert(n.children.cend(),
                                  newChild->children.cbegin(),
                                  newChild->children.cend());
            } else {
                n.children.emplace_back(newChild);
            }
        }

        auto valueChild = std::find_if(node->children.begin(),
                                       node->children.end(),
                                       &isValueSNode);
        if (valueChild != node->children.end()) {
            n.label = materializePTree(contents, (*valueChild)->value);
            n.valueChild = valueChild - node->children.begin();
        } else {
            n.valueChild = -1;
        }

        // move certain nodes onto the next layer
        if (isLayerBreak(n.stype)) {
            Node &nextLevel = tree.makeNode();
            nextLevel.next = &n;
            nextLevel.stype = n.stype;
            nextLevel.label = n.label.empty() ? printSubTree(n) : n.label;
            return &nextLevel;
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
    if (node->next != nullptr) {
        return hashNode(node->next);
    }

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
    if (node.next != nullptr) {
        return hashChildren(*node.next);
    }

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
        if (node.next != nullptr) {
            return visit(*node.next);
        }

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

void
markTreeAsMoved(Node *node)
{
    node->moved = true;

    for (Node *child : node->children) {
        markTreeAsMoved(child);
    }
}
