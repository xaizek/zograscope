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

#include "align.hpp"

#include <deque>
#include <string>
#include <vector>

#include <boost/utility/string_ref.hpp>
#include "dtl/dtl.hpp"

#include "utils/strings.hpp"
#include "tree.hpp"

DiffSource::DiffSource(const Node &root)
{
    struct {
        std::vector<DiceString> &lines;
        std::vector<bool> &modified;
        std::deque<std::string> &storage;

        std::string buffer;
        std::vector<boost::string_ref> spell;
        int line;
        int col;

        void run(const Node &node, bool forceChanged)
        {
            if (node.next != nullptr) {
                forceChanged |= (node.moved || node.state != State::Unchanged);
                return run(*node.next, forceChanged);
            }

            if (node.leaf) {
                if (node.line > line) {
                    if (!buffer.empty()) {
                        storage.push_back(buffer);
                        lines.back() = DiceString(storage.back());
                        buffer.clear();
                    }
                    lines.insert(lines.cend(), node.line - line,
                                 boost::string_ref());
                    modified.insert(modified.cend(), node.line - line, false);
                    line = node.line;
                    col = 1;
                }

                if (node.col > col) {
                    buffer.append(node.col - col, ' ');
                    col = node.col;
                }

                spell.clear();
                split(node.spelling, '\n', spell);
                col += spell.front().size();
                buffer.append(spell.front().cbegin(), spell.front().cend());

                const bool changed = forceChanged
                                  || node.moved
                                  || node.state != State::Unchanged;
                modified.back() = (modified.back() || changed);

                for (std::size_t i = 1U; i < spell.size(); ++i) {
                    ++line;
                    col = 1 + spell[i].size();
                    if (!buffer.empty()) {
                        storage.push_back(buffer);
                        lines.back() = DiceString(storage.back());
                        buffer.clear();
                    }
                    lines.emplace_back(spell[i]);
                    modified.emplace_back(changed);
                }
            }

            for (Node *child : node.children) {
                run(*child, forceChanged);
            }
        }
    } visitor { lines, modified, storage, {}, {}, 0, 1 };

    visitor.run(root, false);
    if (!visitor.buffer.empty()) {
        storage.push_back(std::move(visitor.buffer));
        lines.back() = DiceString(storage.back());
    }
}

std::vector<DiffLine>
makeDiff(DiffSource &&l, DiffSource &&r)
{
    using size_type = std::vector<std::string>::size_type;

    const std::vector<DiceString> &lt = l.lines;
    const std::vector<DiceString> &rt = r.lines;

    auto cmp = [](DiceString &a, DiceString &b) {
        // XXX: hard-coded threshold.
        return (a.compare(b) >= 0.8f);
    };

    dtl::Diff<DiceString, std::vector<DiceString>, decltype(cmp)> diff(lt, rt,
                                                                       cmp);
    diff.compose();

    size_type identicalLines = 0U;
    const size_type minFold = 3;
    const size_type ctxSize = 2;
    std::vector<DiffLine> diffSeq;

    auto foldIdentical = [&](bool last) {
        const size_type startContext =
            (identicalLines == diffSeq.size() ? 0U : ctxSize);
        const size_type endContext = (last ? 0U : ctxSize);
        const size_type context = startContext + endContext;

        if (identicalLines >= context && identicalLines - context > minFold) {
            diffSeq.erase(diffSeq.cend() - (identicalLines - startContext),
                          diffSeq.cend() - endContext);
            diffSeq.emplace(diffSeq.cend() - endContext, Diff::Fold,
                            identicalLines - context);
        }
        identicalLines = 0U;
    };

    auto handleSameLines = [&](size_type i, size_type j) {
        if (lt[i].str() == rt[j].str() && !l.modified[i] && !r.modified[j]) {
            ++identicalLines;
            diffSeq.emplace_back(Diff::Identical);
        } else {
            foldIdentical(false);
            diffSeq.emplace_back(Diff::Different);
        }
    };

    for (const auto &x : diff.getSes().getSequence()) {
        switch (x.second.type) {
            case dtl::SES_DELETE:
                foldIdentical(false);
                diffSeq.emplace_back(Diff::Left);
                break;
            case dtl::SES_ADD:
                foldIdentical(false);
                diffSeq.emplace_back(Diff::Right);
                break;
            case dtl::SES_COMMON:
                handleSameLines(x.second.beforeIdx - 1, x.second.afterIdx - 1);
                break;
        }
    }

    foldIdentical(true);

    return diffSeq;
}
