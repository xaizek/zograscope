#include "TreeBuilder.hpp"

#include <cstddef>

#include <deque>
#include <functional>
#include <utility>
#include <vector>

PNode *
PNode::contract(PNode *node)
{
    if (node->empty() && node->children.size() - node->movedChildren == 1U) {
        // TODO: we could reuse contracted nodes to save some memory
        return contract(node->children[node->movedChildren]);
    }
    return node;
}

PNode *
TreeBuilder::addNode(Text value, const Location &loc, int token, SType stype)
{
    value.token = token;

    if (value.postponedFrom != value.postponedTo) {
        PNode *const node = addNode();
        node->children.reserve(value.postponedTo - value.postponedFrom + 1U);
        for (std::size_t i = value.postponedFrom; i < value.postponedTo; ++i) {
            nodes.emplace_back(postponed[i].value, postponed[i].loc,
                               postponed[i].stype, true);
            node->children.push_back(&nodes.back());
        }
        nodes.emplace_back(value, loc, stype);
        node->children.push_back(&nodes.back());
        return node;
    }

    nodes.emplace_back(value, loc, stype);
    return &nodes.back();
}

PNode *
TreeBuilder::addNode(std::vector<PNode *> children, SType stype)
{
    for (unsigned int i = children.size(); i != 0U; --i) {
        movePostponed(children[i - 1U], children, children.cbegin() + (i - 1U));
    }

    // Contract nodes here to avoid creating node that will be thrown away later
    // in PNode.
    if (stype == SType{} && children.size() == 1U) {
        return PNode::contract(children[0]);
    }

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

    std::function<PNode * (PNode *)> clean = [&clean](PNode *node) {
        std::vector<PNode *> &children = node->children;
        children.erase(children.begin(),
                       children.begin() + node->movedChildren);
        for (PNode *&child : children) {
            child = clean(child);
        }

        node->movedChildren = 0;
        return PNode::contract(node);
    };

    root = clean(root);

    std::vector<PNode *> children;
    children.reserve(newPostponed);
    for (std::size_t i = postponed.size() - newPostponed; i < postponed.size();
         ++i) {
        nodes.emplace_back(postponed[i].value, postponed[i].loc,
                           postponed[i].stype, true);
        children.push_back(&nodes.back());
    }

    root->children.insert(root->children.cend(),
                          children.cbegin(), children.cend());
}

void
TreeBuilder::movePostponed(PNode *&node, std::vector<PNode *> &nodes,
                           std::vector<PNode *>::const_iterator insertPos)
{
    auto pos = std::find_if_not(node->children.begin(), node->children.end(),
                                [](PNode *n) { return n->postponed; });
    if (pos == node->children.begin()) {
        return;
    }

    std::vector<PNode *> postponed(node->children.begin(), pos);
    node->movedChildren = postponed.size();

    if (node->children.end() - pos == 1U && (*pos)->empty() &&
        (*pos)->children.empty()) {
        (*pos)->stype = node->stype;
        node = *pos;
    }

    nodes.insert(insertPos, postponed.cbegin(), postponed.cend());
}
