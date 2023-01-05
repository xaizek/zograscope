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

#ifndef ZOGRASCOPE_UTILS_TREES_HPP_
#define ZOGRASCOPE_UTILS_TREES_HPP_

#include <ostream>
#include <vector>

namespace trees
{

template <typename N>
struct TreeTraits
{
    static unsigned int size(N *node)
    {
        return node->children.size();
    }

    static N * getChild(N *node, unsigned int i)
    {
        return node->children[i];
    }
};

template <typename N, typename F, typename T>
class Printer
{
public:
    Printer(std::ostream &os, const F &nodePrint = &T::print)
        : os(os), nodePrint(nodePrint)
    {
    }

public:
    void print(N *node)
    {
        os << (trace.empty() ? "--- " : "    ");

        for (unsigned int i = 0U, n = trace.size(); i < n; ++i) {
            bool last = (i == n - 1U);
            if (trace[i]) {
                os << (last ? "`-- " : "    ");
            } else {
                os << (last ? "|-- " : "|   ");
            }
        }

        nodePrint(os, node);

        trace.push_back(false);
        for (unsigned int i = 0U, n = T::size(node); i < n; ++i) {
            trace.back() = (i == n - 1U);
            print(T::getChild(node, i));
        }
        trace.pop_back();
    }

private:
    std::ostream &os;
    const F &nodePrint;
    std::vector<bool> trace;
};

template <typename N, typename F, typename T = TreeTraits<N>>
void
print(std::ostream &os, N *node, const F &nodePrinter)
{
    Printer<N, F, T>(os, nodePrinter).print(node);
}

template <typename T, typename N, typename F>
void
printSetTraits(std::ostream &os, N *node, const F &nodePrinter)
{
    Printer<N, F, T>(os, nodePrinter).print(node);
}

}

#endif // ZOGRASCOPE_UTILS_TREES_HPP_
