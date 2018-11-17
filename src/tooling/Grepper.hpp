// Copyright (C) 2018 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZOGRASCOPE__TOOLING__GREPPER_HPP__
#define ZOGRASCOPE__TOOLING__GREPPER_HPP__

#include <string>
#include <utility>
#include <vector>

#include "tree.hpp"

// Finds specified of tokens in a tree.
class Grepper
{
public:
    // Constructs grepper for the specified pattern.
    Grepper(std::vector<std::string> pattern = {});

public:
    // Matches leafs of `node` and invokes `handler` on full match.  Returns
    // `true` if at something was matched or pattern is empty, `false`
    // otherwise.  `handler` must be callable as if it has
    // `void handler(std::vector<Node *> nodes)` signature.
    template <typename F>
    bool grep(Node *node, F &&handler);

    // Checks for empty pattern.
    bool empty() const;

    // Retrieves number of nodes of specified type seen.
    int getSeen() const;
    // Retrieves number of nodes of specified type which were matched.
    int getMatched() const;

private:
    // Implementation of `grep` that calls itself recursively after `grep` did
    // necessary preparations.
    template <typename F>
    bool grepImpl(Node *node, F &&handler);

    // Visits single node.  Returns `true` on finding full match, `false`
    // otherwise.
    template <typename F>
    bool handle(Node *node, F &&handler);

private:
    std::vector<std::string> pattern; // Spellings to match against.
    std::vector<Node *> match;        // Currently matched nodes.

    int seen;    // Number of nodes of specified type seen.
    int matched; // Number of nodes of specified which were matched.
};

inline
Grepper::Grepper(std::vector<std::string> pattern)
    : pattern(std::move(pattern)), seen(0), matched(0)
{
}

template <typename F>
inline bool
Grepper::grep(Node *node, F &&handler)
{
    if (empty()) {
        return true;
    }

    match.clear();
    return grepImpl(node, std::forward<F>(handler));
}

template <typename F>
inline bool
Grepper::grepImpl(Node *node, F &&handler)
{
    if (node->next != nullptr) {
        return grepImpl(node->next, std::forward<F>(handler));
    }

    if (node->children.empty() && node->leaf) {
        return handle(node, std::forward<F>(handler));
    }

    bool foundMatch = false;
    for (Node *child : node->children) {
        if (!child->leaf || child->next != nullptr) {
            foundMatch |= grepImpl(child, std::forward<F>(handler));
        } else {
            foundMatch |= handle(child, std::forward<F>(handler));
        }
    }
    return foundMatch;
}

template <typename F>
inline bool
Grepper::handle(Node *node, F &&handler)
{
    ++seen;

    if (node->spelling != pattern[match.size()]) {
        match.clear();
        return false;
    }

    match.push_back(node);
    if (match.size() == pattern.size()) {
        ++matched;

        handler(match);
        match.clear();

        return true;
    }

    return false;
}

inline bool
Grepper::empty() const
{
    return pattern.empty();
}

inline int
Grepper::getSeen() const
{
    return seen;
}

inline int
Grepper::getMatched() const
{
    return matched;
}

#endif // ZOGRASCOPE__TOOLING__GREPPER_HPP__
