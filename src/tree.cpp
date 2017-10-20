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
#include "stypes.hpp"
#include "trees.hpp"
#include "types.hpp"
#include "utils.hpp"

static void printTree(const Node *node, std::vector<bool> &trace, int depth);
static void printNode(std::ostream &os, const Node *node);
static Node * materializeSNode(Tree &tree, const std::string &contents,
                               const SNode *node);
static std::string materializePTree(const std::string &contents,
                                    const PNode *node);
static Node * materializePNode(Tree &tree, const std::string &contents,
                               const PNode *node);
static std::string materializeLabel(const std::string &contents,
                                    const PNode *node, bool spelling);
static void postOrder(Node &node, std::vector<Node *> &v);
static std::vector<std::size_t> hashChildren(const Node &node);
static std::size_t hashNode(const Node *node);
static void markAsMoved(Node *node);

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

Tree::Tree(const std::string &contents, const PNode *node)
{
    root = materializePNode(*this, contents, node);
}

static bool
shouldSplice(SType parent, Node *childNode)
{
    SType child = childNode->stype;

    if (parent == SType::Statements && child == SType::Statements) {
        return true;
    }

    if (childNode->type == Type::Virtual &&
        child == SType::TemporaryContainer) {
        return true;
    }

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
        case SType::FunctionDeclaration:
        case SType::FunctionDefinition:
        case SType::InitializerElement:
        case SType::InitializerList:
        case SType::Initializer:
        case SType::Declaration:
        case SType::IfExpr:
        case SType::CallExpr:
        case SType::ExprStatement:
        case SType::ReturnValueStmt:
        case SType::Parameter:
        case SType::ForHead:
            return true;

        default:
            return false;
    };
}

Tree::Tree(const std::string &contents, const SNode *node)
{
    root = materializeSNode(*this, contents, node);
}

static Node *
materializeSNode(Tree &tree, const std::string &contents, const SNode *node)
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
            n.next = materializePNode(tree, contents, node->value);
            n.next->last = true;
            n.type = n.next->type;
            return &n;
        }

        n.children.reserve(node->children.size());
        for (SNode *child : node->children) {
            Node *newChild = visit(child);

            if (shouldSplice(node->value->stype, newChild)) {
                if (newChild->next != nullptr) {
                    // Make sure we don't splice last layer.
                    if (newChild->next->last) {
                        n.children.emplace_back(newChild);
                        continue;
                    }

                    newChild = newChild->next;
                }

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

static std::string
materializePTree(const std::string &contents, const PNode *node)
{
    std::string out;

    std::function<void(const PNode *)> visit = [&](const PNode *node) {
        if (node->line != 0 && node->col != 0) {
            out += materializeLabel(contents, node, false);
        }

        for (const PNode *child : node->children) {
            visit(child);
        }
    };
    visit(node);

    return out;
}

static Node *
materializePNode(Tree &tree, const std::string &contents, const PNode *node)
{
    const Type type = tokenToType(node->value.token);

    if (type == Type::Virtual && node->children.size() == 1U) {
        return materializePNode(tree, contents, node->children[0]);
    }

    Node &n = tree.makeNode();
    n.label = materializeLabel(contents, node, false);
    n.spelling = node->stype == SType::Comment
               ? materializeLabel(contents, node, true)
               : n.label;
    n.line = node->line;
    n.col = node->col;
    n.type = type;
    n.stype = node->stype;

    n.children.reserve(node->children.size());
    for (const PNode *child : node->children) {
        n.children.push_back(materializePNode(tree, contents, child));
    }

    return &n;
}

static std::string
materializeLabel(const std::string &contents, const PNode *node, bool spelling)
{
    // XXX: lexer also has such variable and they need to be synchronized
    //      (actually we should pass this to lexer).
    enum { tabWidth = 4 };

    std::string label;
    label.reserve(node->value.len);

    bool leadingWhitespace = false;
    int col = node->col;
    for (std::size_t i = node->value.from;
         i < node->value.from + node->value.len; ++i) {
        switch (const char c = contents[i]) {
            int width;

            case '\n':
                col = 1;
                label += '\n';
                leadingWhitespace = !spelling
                                 && node->stype == SType::Comment;
                break;
            case '\t':
                width = tabWidth - (col - 1)%tabWidth;
                col += width;
                if (!leadingWhitespace) {
                    label.append(width, ' ');
                }
                break;
            case ' ':
                ++col;
                if (!leadingWhitespace) {
                    label += ' ';
                }
                break;

            default:
                ++col;
                label += c;
                leadingWhitespace = false;
                break;
        }
    }

    return label;
}

std::vector<Node *>
postOrder(Node &root)
{
    std::vector<Node *> v;
    root.parent = &root;
    postOrder(root, v);
    return v;
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

std::string
printSubTree(const Node &root)
{
    std::string out;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.next != nullptr) {
            return visit(*node.next);
        }

        if (node.line != 0 && node.col != 0) {
            out += node.label;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };
    visit(root);

    return out;
}

bool
canBeFlattened(const Node *parent, const Node *child)
{
    return child->stype != SType::Declaration
        || parent->stype != SType::TranslationUnit;
}

bool
isUnmovable(const Node *x)
{
    return x->stype == SType::Statements
        || x->stype == SType::Bundle
        || x->stype == SType::BundleComma
        || (x->stype == SType::Statement && x->type == Type::Virtual);
}

bool
hasMoveableItems(const Node *x)
{
    return (!isUnmovable(x) || isContainer(x));
}

bool
isContainer(const Node *x)
{
    return x->stype == SType::Statements
        || x->stype == SType::Bundle
        || x->stype == SType::BundleComma;
}

bool
isExpr(const Node *x)
{
    return x->stype == SType::ExprStatement
        || x->stype == SType::IfExpr;
}

void
markTreeAsMoved(Node *node)
{
    if (hasMoveableItems(node)) {
        markAsMoved(node);
    }
}

static void
markAsMoved(Node *node)
{
    node->moved = !isUnmovable(node);

    for (Node *child : node->children) {
        markAsMoved(child);
    }
}
