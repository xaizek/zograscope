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
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZOGRASCOPE__LEAFRANGE_HPP__
#define ZOGRASCOPE__LEAFRANGE_HPP__

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
struct LeafRangeData
{
    // Initializes data from a node.
    explicit LeafRangeData(const Node *node)
    {
        toVisit.emplace(node);
        current = nullptr;
    }

    std::stack<const Node *> toVisit; // Unvisited nodes.
    const Node *current;              // Current node.
};

// Iterator over leaves.
class LeafIterator
  : public boost::iterator_facade<
        LeafIterator,
        const Node *,
        std::input_iterator_tag
    >
{
    friend class boost::iterator_core_access;

public:
    // Constructs empty (end) iterator.
    LeafIterator() : rd(nullptr)
    { }

    // Constructs non-empty iterator.
    LeafIterator(LeafRangeData *rd) : rd(rd)
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

            if (node.leaf) {
                rd->current = &node;
                return;
            }

            for (const Node *child : boost::adaptors::reverse(node.children)) {
                rd->toVisit.push(child);
            }
        }

        // Turn into end iterator.
        *this = LeafIterator();
    }

    // Checks whether two iterators are equal.
    bool equal(const LeafIterator &that) const
    { return rd == that.rd; }

    // Retrieves value of a valid iterator (not end iterator).
    const Node * & dereference() const
    { return rd->current; }

private:
    // Pointer to data storage.
    LeafRangeData *rd;
};

using LeafRangeBase = boost::iterator_range<LeafIterator>;

}

// Range iterator over leaves of a tree.
class LeafRange : private guts::LeafRangeData, public guts::LeafRangeBase
{
public:
    explicit LeafRange(const Node *node)
        : guts::LeafRangeData(node),
          guts::LeafRangeBase(guts::LeafIterator(this), guts::LeafIterator())
    { }
};

#endif // ZOGRASCOPE__LEAFRANGE_HPP__
