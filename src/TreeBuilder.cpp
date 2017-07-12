#include "TreeBuilder.hpp"

#include <cassert>
#include <cstddef>

#include <deque>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include "trees.hpp"

static std::ostream &
operator<<(std::ostream &os, SType stype)
{
    switch (stype) {
        case SType::None:                return (os << "None");
        case SType::TranslationUnit:     return (os << "TranslationUnit");
        case SType::Declaration:         return (os << "Declaration");
        case SType::FunctionDeclaration: return (os << "FunctionDeclaration");
        case SType::FunctionDefinition:  return (os << "FunctionDefinition");
        case SType::Postponed:           return (os << "Postponed");
        case SType::Macro:               return (os << "Macro");
        case SType::CompoundStatement:   return (os << "CompoundStatement");
        case SType::Separator:           return (os << "Separator");
        case SType::Statements:          return (os << "Statements");
        case SType::Statement:           return (os << "Statement");
        case SType::IfStmt:              return (os << "IfStmt");
        case SType::IfCond:              return (os << "IfCond");
        case SType::IfElse:              return (os << "IfElse");
        case SType::WhileStmt:           return (os << "WhileStmt");
        case SType::WhileCond:           return (os << "WhileCond");
        case SType::ForStmt:             return (os << "ForStmt");
        case SType::ForHead:             return (os << "ForHead");
        case SType::Expression:          return (os << "Expression");
        case SType::Declarator:          return (os << "Declarator");
        case SType::Initializer:         return (os << "Initializer");
        case SType::Specifiers:          return (os << "Specifiers");
        case SType::WithInitializer:     return (os << "WithInitializer");
        case SType::InitializerElement:  return (os << "InitializerElement");
    }

    assert("Unhandled enumeration item");
    return (os << "<UNKNOWN>");
}

PNode *
TreeBuilder::addNode(Text value, const Location &loc, int token, SType stype)
{
    value.token = token;

    if (value.postponedFrom != value.postponedTo) {
        std::vector<PNode *> children;
        children.reserve(value.postponedTo - value.postponedFrom);
        for (std::size_t i = value.postponedFrom; i < value.postponedTo; ++i) {
            nodes.emplace_back(postponed[i].value, postponed[i].loc,
                               SType::Postponed, true);
            children.push_back(&nodes.back());
        }
        nodes.emplace_back(value, loc, stype);
        children.push_back(&nodes.back());
        nodes.emplace_back(std::move(children));
        return &nodes.back();
    }

    nodes.emplace_back(value, loc, stype);
    return &nodes.back();
}

PNode *
TreeBuilder::addNode(std::vector<PNode *> children, SType stype)
{
    movePostponed(children[0], children, children.cbegin());
    nodes.emplace_back(std::move(children), stype);
    return &nodes.back();
}

void
TreeBuilder::finish(bool failed)
{
    if (failed) {
        this->failed = failed;
        return;
    }

    std::function<void(PNode *)> clean = [&](PNode *node) {
        std::vector<PNode *> &children = node->children;
        children.erase(children.begin(),
                       children.begin() + node->movedChildren);
        for (PNode *child : children) {
            clean(child);
        }
    };

    clean(root);

    std::vector<PNode *> children;
    children.reserve(newPostponed);
    for (std::size_t i = postponed.size() - newPostponed; i < postponed.size();
         ++i) {
        nodes.emplace_back(postponed[i].value, postponed[i].loc,
                           SType::Postponed, true);
        children.push_back(&nodes.back());
    }

    root->children.insert(root->children.cend(),
                          children.cbegin(), children.cend());
}

static void
print(const PNode *node, const std::string &contents)
{
    trees::print(std::cout, node,
                 [&contents](std::ostream &os, const PNode *node) {
                     os << contents.substr(node->value.from, node->value.len)
                        << " (" << node->stype << ")\n";
                 });
}

static PNode *
findSNode(PNode *node)
{
    if (node->stype != SType::None) {
        return node;
    }

    return node->children.size() == 1U
         ? findSNode(node->children.front())
         : nullptr;
}

SNode *
TreeBuilder::makeSTree(const std::string &contents, bool dumpWhole,
                       bool dumpUnclear)
{
    if (dumpWhole) {
        print(root, contents);
    }

    PNode *rootNode = findSNode(root);
    if (rootNode == nullptr) {
        snodes.emplace_back(SNode{root, {}});
        return &snodes[0];
    }

    std::function<SNode *(PNode *)> makeSNode = [&, this](PNode *node) {
        auto isSNode = [](PNode *child) {
            return (findSNode(child) != nullptr);
        };
        // If none of the children is SNode, then current node is a leaf SNode.
        if (std::none_of(node->children.begin(), node->children.end(),
                         isSNode)) {
            snodes.emplace_back(SNode{node, {}});
            return &snodes.back();
        }

        std::vector<SNode *> c;
        c.reserve(node->children.size());
        for (PNode *child : node->children) {
            if (PNode *schild = findSNode(child)) {
                c.push_back(makeSNode(schild));
            } else {
                if (dumpUnclear) {
                    print(child, contents);
                }
                snodes.emplace_back(SNode{node->children[c.size()], {}});
                c.push_back(&snodes.back());
            }
        }
        snodes.emplace_back(SNode{node, std::move(c)});
        return &snodes.back();
    };

    return makeSNode(rootNode);
}

void
TreeBuilder::movePostponed(PNode *&node, std::vector<PNode *> &nodes,
                           std::vector<PNode *>::const_iterator insertPos)
{
    auto pos = std::partition_point(node->children.begin(),
                                    node->children.end(),
                                    [](PNode *n) { return n->postponed; });
    if (pos == node->children.begin()) {
        return;
    }

    std::vector<PNode *> postponed(node->children.begin(), pos);
    node->movedChildren = postponed.size();

    if (node->children.end() - pos == 1U && (*pos)->empty()) {
        (*pos)->stype = node->stype;
        node = *pos;
    }

    nodes.insert(insertPos, postponed.cbegin(), postponed.cend());
}
