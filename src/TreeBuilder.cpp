// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "TreeBuilder.hpp"

#include <cstddef>

#include <functional>
#include <utility>

#include "utils/Pool.hpp"

static PNode * shrinkTree(PNode *node);

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
            node->children.push_back(pool.make(postponed[i].value,
                                               postponed[i].loc,
                                               postponed[i].stype,
                                               true));
        }
        node->children.push_back(pool.make(value, loc, stype, false));
        return node;
    }

    return pool.make(value, loc, stype, false);
}

PNode *
TreeBuilder::addNode(const std::initializer_list<PNode *> &ini, SType stype)
{
    cpp17::pmr::vector<PNode *> children(ini, alloc);

    // Lifts postponed nodes from children inserting them right before them in
    // children's list of their future parent.
    for (unsigned int i = children.size(); i != 0U; --i) {
        movePostponed(children[i - 1U], children, children.cbegin() + (i - 1U));
    }

    // Contract nodes here to avoid creating node that will be thrown away later
    // in PNode.
    if (stype == SType{} && children.size() == 1U) {
        return PNode::contract(children[0]);
    }

    return pool.make(std::move(children), stype);
}

void
TreeBuilder::finish(bool failed)
{
    if (failed) {
        this->failed = failed;
        return;
    }

    root = shrinkTree(root);

    for (std::size_t i = postponed.size() - newPostponed; i < postponed.size();
         ++i) {
        root->children.push_back(pool.make(postponed[i].value, postponed[i].loc,
                                           postponed[i].stype, true));
    }
}

// Drops children of each node within the tree that were "moved" to some parent
// nodes.  Returns contracted node.
static PNode *
shrinkTree(PNode *node)
{
    cpp17::pmr::vector<PNode *> &children = node->children;
    children.erase(children.begin(),
                   children.begin() + node->movedChildren);
    for (PNode *&child : children) {
        child = shrinkTree(child);
    }

    node->movedChildren = 0;
    return PNode::contract(node);
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

    node->movedChildren = pos - node->children.begin();
    PNode *n = node;
    node = PNode::contract(n);

    nodes.insert(insertPos, n->children.begin(), pos);
}
