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

#ifndef ZOGRASCOPE__TOOLING__MATCHER_HPP__
#define ZOGRASCOPE__TOOLING__MATCHER_HPP__

#include <cstdint>

#include <utility>

#include "Language.hpp"
#include "tree.hpp"

enum class MType : std::uint8_t;

// Finds a node of specified type that matches another matcher.
class Matcher
{
public:
    // Constructs matcher that matches `mtype` and delegates nested matching to
    // `nested`.  `nested` matcher can be `nullptr`, in which case `handler`
    // passed to `match()` is called.
    Matcher(MType mtype, Matcher *nested);

public:
    // Matches children of `node` and invokes `handler` on the last match in the
    // chain.  Returns `true` if at something was matched, `false` otherwise.
    // `handler` must be callable as if it has `void handler(const Node *node)`
    // signature.
    template <typename F>
    bool match(const Node *node, Language &lang, F &&handler);

    // Retrieves type this matcher matches against.
    MType getMType() const;
    // Retrieves number of nodes of specified type seen.
    int getSeen() const;
    // Retrieves number of nodes of specified type which were matched.
    int getMatched() const;

private:
    MType mtype;     // Type to match against.
    Matcher *nested; // Nested matcher, can be `nullptr`.
    int seen;        // Number of nodes of specified type seen.
    int matched;     // Number of nodes of specified which were matched.
};

inline
Matcher::Matcher(MType mtype, Matcher *nested)
    : mtype(mtype), nested(nested), seen(0), matched(0)
{
}

template <typename F>
inline bool
Matcher::match(const Node *node, Language &lang, F &&handler)
{
    if (node->next != nullptr) {
        return match(node->next, lang, std::forward<F>(handler));
    }

    bool foundMatch = false;
    for (Node *child : node->children) {
        if (lang.classify(child->stype) != mtype) {
            foundMatch |= match(child, lang, std::forward<F>(handler));
            continue;
        }

        ++seen;
        if (nested == nullptr ||
            nested->match(child, lang, std::forward<F>(handler))) {
            ++matched;
            foundMatch = true;

            if (nested == nullptr) {
                handler(child);
            }
        }
    }
    return foundMatch;
}

inline MType
Matcher::getMType() const
{
    return mtype;
}

inline int
Matcher::getSeen() const
{
    return seen;
}

inline int
Matcher::getMatched() const
{
    return matched;
}

#endif // ZOGRASCOPE__TOOLING__MATCHER_HPP__
