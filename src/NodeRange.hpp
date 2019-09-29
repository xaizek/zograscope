// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__NODERANGE_HPP__
#define ZOGRASCOPE__NODERANGE_HPP__

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stack>
#include <string>
#include <utility>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/iterator_range.hpp>

#include "tree.hpp"

namespace guts {

// Storage for range iterator data.
struct NodeRangeData
{
    // Initializes data from a node.
    explicit NodeRangeData(const Node *node)
    {
        toVisit.emplace(node);
        current = nullptr;
    }

    std::stack<const Node *> toVisit; // Unvisited nodes.
    const Node *current;              // Current node.
};

// Iterator over all nodes.
class NodeIterator
  : public boost::iterator_facade<
        NodeIterator,
        const Node *,
        std::input_iterator_tag
    >
{
    friend class boost::iterator_core_access;

public:
    // Constructs empty (end) iterator.
    NodeIterator() : rd(nullptr)
    { }

    // Constructs non-empty iterator.
    NodeIterator(NodeRangeData *rd) : rd(rd)
    { increment(); }

private:
    // Advances to the next node.
    void increment()
    {
        while (!rd->toVisit.empty()) {
            const Node &node = *rd->toVisit.top();
            rd->toVisit.pop();

            if (node.next != nullptr) {
                rd->toVisit.push(node.next);
                continue;
            }

            for (const Node *child : boost::adaptors::reverse(node.children)) {
                rd->toVisit.push(child);
            }

            rd->current = &node;
            return;
        }

        // Turn into end iterator.
        *this = NodeIterator();
    }

    // Checks whether two iterators are equal.
    bool equal(const NodeIterator &that) const
    { return rd == that.rd; }

    // Retrieves value of a valid iterator (not end iterator).
    const Node * & dereference() const
    { return rd->current; }

private:
    // Pointer to data storage.
    NodeRangeData *rd;
};

using NodeRangeBase = boost::iterator_range<NodeIterator>;

}

// Range iterator over all nodes of a tree.
class NodeRange : private guts::NodeRangeData, public guts::NodeRangeBase
{
public:
    explicit NodeRange(const Node *node)
        : guts::NodeRangeData(node),
          guts::NodeRangeBase(guts::NodeIterator(this), guts::NodeIterator())
    { }
};

#endif // ZOGRASCOPE__NODERANGE_HPP__
