#include "TreeBuilder.hpp"

#include <cstddef>

#include <algorithm>
#include <deque>
#include <utility>
#include <vector>

PNode *
TreeBuilder::addNode(Text value, const Location &loc, int token)
{
    value.token = token;

    if (value.postponedFrom != value.postponedTo) {
        std::vector<PNode *> children;
        children.reserve(value.postponedTo - value.postponedFrom);
        for (std::size_t i = value.postponedFrom; i < value.postponedTo; ++i) {
            nodes.emplace_back(postponed[i].value, postponed[i].loc, true);
            children.push_back(&nodes.back());
        }
        nodes.emplace_back(value, loc);
        children.push_back(&nodes.back());
        nodes.emplace_back(std::move(children));
        return &nodes.back();
    }

    nodes.emplace_back(value, loc);
    return &nodes.back();
}

void
TreeBuilder::finish(bool failed)
{
    if (failed) {
        this->failed = failed;
        return;
    }

    std::vector<PNode *> children;
    children.reserve(newPostponed);
    for (std::size_t i = postponed.size() - newPostponed;
            i < postponed.size(); ++i) {
        nodes.emplace_back(postponed[i].value, postponed[i].loc, true);
        children.push_back(&nodes.back());
    }

    root->children.insert(root->children.cend(),
                            children.cbegin(), children.cend());
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
    node->children.erase(node->children.begin(), pos);

    if (node->children.front()->empty() && node->children.size() == 1U) {
        node = node->children.front();
    }

    nodes.insert(insertPos, postponed.cbegin(), postponed.cend());
}
