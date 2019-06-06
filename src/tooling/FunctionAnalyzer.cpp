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

#include "FunctionAnalyzer.hpp"

#include "LeafRange.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

FunctionAnalyzer::FunctionAnalyzer(Language &lang) : lang(lang)
{ }

int
FunctionAnalyzer::getLineCount(const Node *node) const
{
    LeafRange range(node);
    auto curr = range.begin();
    if (curr == range.end()) {
        return 0;
    }

    int startLine = (*curr)->line;
    int endLine;
    do {
        endLine = (*curr)->line;
    } while (++curr != range.end());

    return endLine - startLine + 1;
}

int
FunctionAnalyzer::getParamCount(const Node *node) const
{
    if (node->next != nullptr) {
        return getParamCount(node->next);
    }

    int paramCount = 0;
    for (const Node *child : node->children) {
        MType mtype = lang.classify(child->stype);
        if (mtype == MType::Parameter && !lang.isPseudoParameter(child)) {
            ++paramCount;
        } else if (mtype == MType::Declaration || mtype == MType::Other) {
            paramCount += getParamCount(child);
        }
    }
    return paramCount;
}
