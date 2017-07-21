#include "STree.hpp"

#include <deque>
#include <iostream>
#include <utility>
#include <vector>

#include "trees.hpp"

static void print(const PNode *node, const std::string &contents);
static PNode * findSNode(PNode *node);
static SNode * makeSNode(std::deque<SNode> &snodes, const std::string &contents,
                         PNode *pnode, bool dumpUnclear);

STree::STree(TreeBuilder &&ptree, const std::string &contents, bool dumpWhole,
             bool dumpUnclear)
    : ptree(std::move(ptree))
{
    PNode *proot = ptree.getRoot();

    if (dumpWhole) {
        print(proot, contents);
    }

    PNode *rootNode = findSNode(proot);
    if (rootNode == nullptr) {
        snodes.emplace_back(SNode{proot, {}});
        root = &snodes[0];
        return;
    }

    root = makeSNode(snodes, contents, rootNode, dumpUnclear);
}

static void
print(const PNode *node, const std::string &contents)
{
    trees::print(std::cout, node,
                 [&contents](std::ostream &os, const PNode *node) {
                     os << contents.substr(node->value.from, node->value.len)
                        << " (SType::" << node->stype << ")\n";
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

static SNode *
makeSNode(std::deque<SNode> &snodes, const std::string &contents, PNode *pnode,
          bool dumpUnclear)
{
    auto isSNode = [](PNode *child) {
        return (findSNode(child) != nullptr);
    };
    // If none of the children is SNode, then current node is a leaf SNode.
    if (std::none_of(pnode->children.begin(), pnode->children.end(),
                     isSNode)) {
        snodes.emplace_back(SNode{pnode, {}});
        return &snodes.back();
    }

    std::vector<SNode *> c;
    c.reserve(pnode->children.size());
    for (PNode *child : pnode->children) {
        if (PNode *schild = findSNode(child)) {
            c.push_back(makeSNode(snodes, contents, schild, dumpUnclear));
        } else {
            if (dumpUnclear) {
                print(child, contents);
            }
            snodes.emplace_back(SNode{pnode->children[c.size()], {}});
            c.push_back(&snodes.back());
        }
    }
    snodes.emplace_back(SNode{pnode, std::move(c)});
    return &snodes.back();
}
