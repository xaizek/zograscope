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

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include <boost/utility/string_ref.hpp>
#include "dtl/dtl.hpp"

#include "utils/CountIterator.hpp"
#include "utils/strings.hpp"
#include "tree.hpp"

DiffSource::DiffSource(const Node &root)
{
    struct {
        std::vector<LineInfo> &lines;
        std::vector<bool> &modified;
        std::deque<std::string> &storage;

        std::string buffer;
        std::vector<boost::string_ref> spell;
        int line;
        int col;

        void run(const Node &node, bool forceChanged,
                 const Node *n, const Node *relative)
        {
            if (node.next != nullptr) {
                forceChanged |= (node.moved || node.state != State::Unchanged);
                n = (node.next->last ? &node : nullptr);
                relative = (node.relative != nullptr ? node.relative : nullptr);
                return run(*node.next, forceChanged, n, relative);
            }

            if (node.leaf) {
                if (node.line > line) {
                    if (!buffer.empty()) {
                        storage.push_back(buffer);
                        lines.back().text = DiceString(storage.back());
                        buffer.clear();
                    }
                    lines.insert(lines.cend(), node.line - line, LineInfo());
                    modified.insert(modified.cend(), node.line - line, false);
                    line = node.line;
                    col = 1;
                }

                if (node.col > col) {
                    buffer.append(node.col - col, ' ');
                    col = node.col;
                }

                const Node *leafN = (node.relative == nullptr ? n : &node);
                const Node *leafRelative = (node.relative != nullptr)
                                         ? node.relative
                                         : relative;

                spell.clear();
                split(node.spelling, '\n', spell);
                col += spell.front().size();
                buffer.append(spell.front().cbegin(), spell.front().cend());
                lines.back().nodes.push_back(leafN);
                lines.back().rels.push_back(leafRelative);

                const bool changed = forceChanged
                                  || node.moved
                                  || node.state != State::Unchanged;
                modified.back() = (modified.back() || changed);

                for (std::size_t i = 1U; i < spell.size(); ++i) {
                    ++line;
                    col = 1 + spell[i].size();
                    if (!buffer.empty()) {
                        storage.push_back(buffer);
                        lines.back().text = DiceString(storage.back());
                        buffer.clear();
                    }
                    lines.emplace_back(spell[i], leafN, leafRelative);
                    modified.emplace_back(changed);
                }
            }

            for (Node *child : node.children) {
                run(*child, forceChanged, n, relative);
            }
        }
    } visitor { lines, modified, storage, {}, {}, 0, 1 };

    visitor.run(root, false, nullptr, nullptr);
    if (!visitor.buffer.empty()) {
        storage.push_back(std::move(visitor.buffer));
        lines.back().text = DiceString(storage.back());
    }
}

std::vector<DiffLine>
makeDiff(DiffSource &&l, DiffSource &&r)
{
    using size_type = std::vector<std::string>::size_type;

    std::vector<LineInfo> &lt = l.lines;
    std::vector<LineInfo> &rt = r.lines;

    for (LineInfo &info : lt) {
        std::sort(info.rels.begin(), info.rels.end());
    }
    for (LineInfo &info : rt) {
        std::sort(info.nodes.begin(), info.nodes.end());
    }

    auto cmp = [](LineInfo &a, LineInfo &b) {
        int all = a.rels.size() + b.nodes.size();
        if (all == 0) {
            // Match empty lines.
            return true;
        }

        auto skipNulls = [](std::vector<const Node *> &v) {
            return std::find_if(v.cbegin(), v.cend(), [](const Node *n) {
                return (n != nullptr);
            });
        };

        auto aRels = skipNulls(a.rels);
        auto bRels = skipNulls(b.rels);
        auto bNodes = skipNulls(b.nodes);

        int matched = std::set_intersection(aRels, a.rels.cend(),
                                            bNodes, b.nodes.cend(),
                                            CountIterator()).getCount();
        int total = (a.rels.cend() - aRels) + (b.nodes.cend() - bNodes);
        // XXX: hard-coded thresholds.
        return false
            // Check for matched tokens first.
            || (total != 0 && 2.0f*matched/total >= 0.6f)
            // Check for complete replacement of tokens which look alike a bit.
            || (matched == 0 &&
                aRels == a.rels.cend() && bRels == b.rels.cend() &&
                !a.nodes.empty() && !b.nodes.empty() &&
                a.text.compare(b.text) >= 0.4f)
            // Resort to text based comparison for small total number of tokens
            // unless one of lines contains only removed/added tokens (first
            // condition).
            || ((aRels == a.rels.cend()) == (bRels == b.rels.cend()) &&
                all > 2 && all < 7 && a.text.compare(b.text) >= 0.8f);
    };

    dtl::Diff<LineInfo, std::vector<LineInfo>, decltype(cmp)> diff(lt, rt, cmp);
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
        if (!l.modified[i] && !r.modified[j] &&
            lt[i].text.str() == rt[j].text.str()) {
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
