#include "TreeBuilder.hpp"

#include <cstddef>

#include <functional>
#include <utility>

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
        nodes.emplace_back(value, loc, stype, false);
        node->children.push_back(&nodes.back());
        return node;
    }

    nodes.emplace_back(value, loc, stype, false);
    return &nodes.back();
}

PNode *
TreeBuilder::addNode(const std::initializer_list<PNode *> &ini, SType stype)
{
    cpp17::pmr::vector<PNode *> children(ini, alloc);

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
        cpp17::pmr::vector<PNode *> &children = node->children;
        children.erase(children.begin(),
                       children.begin() + node->movedChildren);
        for (PNode *&child : children) {
            child = clean(child);
        }

        node->movedChildren = 0;
        return PNode::contract(node);
    };

    root = clean(root);

    for (std::size_t i = postponed.size() - newPostponed; i < postponed.size();
         ++i) {
        nodes.emplace_back(postponed[i].value, postponed[i].loc,
                           postponed[i].stype, true);
        root->children.push_back(&nodes.back());
    }
}

void
TreeBuilder::movePostponed(PNode *&node, cpp17::pmr::vector<PNode *> &nodes,
                          cpp17::pmr::vector<PNode *>::const_iterator insertPos)
{
    auto pos = std::find_if_not(node->children.begin(), node->children.end(),
                                [](PNode *n) { return n->postponed; });
    if (pos == node->children.begin()) {
        return;
    }

    // Save the value, because it's modified below.
    PNode *n = node;

    if (node->children.end() - pos == 1U && (*pos)->empty() &&
        (*pos)->children.empty()) {
        (*pos)->stype = node->stype;
        node = *pos;
    }

    // Inserting elements here to avoid invalidating pos iterator.
    nodes.insert(insertPos, n->children.begin(), pos);
    n->movedChildren = pos - n->children.begin();
}
