// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#include "Highlighter.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>
#include "dtl/dtl.hpp"

#include "utils/strings.hpp"
#include "ColorScheme.hpp"
#include "decoration.hpp"
#include "stypes.hpp"
#include "tree.hpp"

static const decor::Decoration & getHighlight(const Node &node, int moved,
                                              State state);
static bool isDiffable(const Node &node, State state);
static std::string diffSpelling(const std::string &l, const std::string &r,
                                const decor::Decoration &dec, bool original);
static std::vector<boost::string_ref> toWords(const std::string &s);
static std::string getSpelling(const Node &node, State state,
                               const decor::Decoration &dec, bool original);

// Single processing entry.
struct Highlighter::Entry
{
    Node *node;          // Node to be processed.
    bool moved;          // Move status override for descendants.
    State state;         // State override for descendants.
    bool propagateMoved; // Moved field should be passed to all descendants.
    bool propagateState; // State field should be passed to all descendants.
};

class Highlighter::ColorPicker
{
public:
    void setEntry(const Entry &entry)
    {
        prevNode = (currNode == entry.node ? nullptr : currNode);
        currNode = entry.node;

        prevHighlight = currHighlight;
        currHighlight = &::getHighlight(*currNode, entry.moved, entry.state);
    }

    void advancedLine()
    {
        prevNode = nullptr;
    }

    const decor::Decoration & getHighlight() const
    {
        return *currHighlight;
    }

    const decor::Decoration & getFillHighlight() const
    {
        if (prevNode == nullptr) {
            return decor::none;
        }

        if (prevHighlight == currHighlight) {
            return *currHighlight;
        }

        if (prevNode->moved || currNode->moved) {
            return (currNode->moved ? *prevHighlight : *currHighlight);
        }

        return decor::none;
    }

private:
    const decor::Decoration *currHighlight = &decor::none;
    const decor::Decoration *prevHighlight = &decor::none;
    const Node *currNode = nullptr;
    const Node *prevNode = nullptr;
};

Highlighter::Highlighter(Node &root, bool original)
    : line(1), col(1), colorPicker(new ColorPicker()), original(original),
      current(nullptr)
{
    toProcess.push({ &root, root.moved, root.state, false, false });
}

Highlighter::~Highlighter()
{
    // Emit destructor here with ColorPicker and Entry classes being complete.
}

std::string
Highlighter::print(int from, int n)
{
    if (from < line) {
        n = std::max(0, n - (line - from));
    }

    skipUntil(from);
    print(n);

    std::string result = oss.str();
    oss.str(std::string());
    return result;
}

std::string
Highlighter::print()
{
    print(std::numeric_limits<int>::max());

    std::string result = oss.str();
    oss.str(std::string());
    return result;
}

void
Highlighter::skipUntil(int targetLine)
{
    if (line >= targetLine) {
        return;
    }

    if (!lines.empty()) {
        for (std::size_t i = 1U; i < lines.size(); ++i) {
            if (++line == targetLine) {
                olines.erase(olines.begin(), olines.begin() + i);
                lines.erase(lines.begin(), lines.begin() + i);
                return;
            }
        }
        olines.clear();
        lines.clear();
    }

    while (!toProcess.empty()) {
        Entry entry = getEntry();
        Node *const node = entry.node;
        if (!node->leaf) {
            advance(entry);
            continue;
        }

        colorPicker->setEntry(entry);

        while (node->line > line) {
            if (++line == targetLine) {
                return;
            }
        }

        advance(entry);

        split(node->spelling, '\n', olines);

        for (std::size_t i = 1U; i < olines.size(); ++i) {
            if (++line == targetLine) {
                current = node;
                colorPicker->advancedLine();
                const decor::Decoration &dec = colorPicker->getHighlight();
                spelling = getSpelling(*node, entry.state, dec, original);
                split(spelling, '\n', lines);
                olines.erase(olines.begin(), olines.begin() + i);
                lines.erase(lines.begin(), lines.begin() + i);
                return;
            }
        }

        olines.clear();
        lines.clear();
    }
}

void
Highlighter::print(int n)
{
    col = 1;
    colorPicker->advancedLine();

    if (!lines.empty()) {
        printSpelling(n);
    }

    while (!toProcess.empty() && n != 0) {
        Entry entry = getEntry();
        Node *const node = entry.node;
        if (!node->leaf) {
            advance(entry);
            continue;
        }

        colorPicker->setEntry(entry);

        const decor::Decoration &fillHighlight =
            colorPicker->getFillHighlight();
        // Don't do filling across lines.
        if (node->line == line && &fillHighlight != &decor::none) {
            oss << fillHighlight;
        }

        while (node->line > line) {
            ++line;
            colorPicker->advancedLine();
            if (--n == 0) {
                return;
            }
            oss << '\n';
            col = 1;
        }

        while (node->col > col) {
            oss << ' ';
            ++col;
        }

        if (&fillHighlight != &decor::none) {
            oss << decor::def;
        }

        advance(entry);

        spelling = getSpelling(*node, entry.state, colorPicker->getHighlight(),
                               original);
        split(node->spelling, '\n', olines);
        split(spelling, '\n', lines);
        current = node;

        printSpelling(n);
    }
}

void
Highlighter::printSpelling(int &n)
{
    const decor::Decoration &dec = colorPicker->getHighlight();

    auto printLine = [&](boost::string_ref line) {
        if (current->line == this->line) {
            oss << (dec << lines.front());
            return;
        }

        std::size_t whitespaceLength = line.find_first_not_of(" \t");
        if (whitespaceLength == std::string::npos) {
            whitespaceLength = line.size();
        }
        oss << line.substr(0, whitespaceLength)
            << (dec << line.substr(whitespaceLength));
    };

    printLine(lines.front());
    col += olines.front().size();

    for (std::size_t i = 1U; i < lines.size(); ++i) {
        ++line;
        colorPicker->advancedLine();
        if (--n == 0) {
            olines.erase(olines.begin(), olines.begin() + i);
            lines.erase(lines.begin(), lines.begin() + i);
            return;
        }

        oss << '\n';
        printLine(lines[i]);
        col = 1 + olines[i].size();
    }

    olines.clear();
    lines.clear();
}

Highlighter::Entry
Highlighter::getEntry()
{
    Entry entry = toProcess.top();
    Node *node = entry.node;

    if (node->next != nullptr || node->leaf) {
        if (!entry.propagateState && node->state != State::Unchanged) {
            entry.propagateState = true;
            entry.state = node->state;
        }
        if (node->moved) {
            entry.propagateMoved = true;
            entry.moved = true;
        }
    }

    if (node->next != nullptr) {
        // Update entry and rerun the checks (state can be changed both on
        // switching to a different layer and directly after switching to it).
        entry.node = node->next;
        toProcess.top() = entry;
        return getEntry();
    }

    return entry;
}

void
Highlighter::advance(const Entry &entry)
{
    toProcess.pop();
    for (Node *child : boost::adaptors::reverse(entry.node->children)) {
        Entry childEntry = entry;
        childEntry.node = child;
        if (!childEntry.propagateState) {
            childEntry.state = child->state;
        }
        if (!childEntry.propagateMoved) {
            childEntry.moved = child->moved;
        }

        toProcess.push(childEntry);
    }
}

// Retrieves decoration for the specified node considering overrides of its
// properties.
static const decor::Decoration &
getHighlight(const Node &node, int moved, State state)
{
    static ColorScheme cs;

    // Highlighting based on node state has higher priority.
    switch (state) {
        case State::Deleted:
            return cs[ColorGroup::Deleted];
        case State::Inserted:
            return cs[ColorGroup::Inserted];
        case State::Updated:
            // TODO: things that were updated and moved at the same time need a
            //       special color?
            //       Or update kinda implies move, since it's obvious that there
            //       was a match between nodes?
            if (!isDiffable(node, state)) {
                return cs[ColorGroup::Updated];
            }
            break;

        case State::Unchanged:
            if (moved) {
                return cs[ColorGroup::Moved];
            }
            break;
    }

    // Highlighting based on node type follows.
    switch (node.type) {
        case Type::Specifiers: return cs[ColorGroup::Specifiers];
        case Type::UserTypes:  return cs[ColorGroup::UserTypes];
        case Type::Types:      return cs[ColorGroup::Types];
        case Type::Directives: return cs[ColorGroup::Directives];
        case Type::Comments:   return cs[ColorGroup::Comments];
        case Type::Functions:  return cs[ColorGroup::Functions];

        case Type::Jumps:
        case Type::Keywords:
            return cs[ColorGroup::Keywords];
        case Type::LeftBrackets:
        case Type::RightBrackets:
            return cs[ColorGroup::Brackets];
        case Type::Assignments:
        case Type::Operators:
        case Type::LogicalOperators:
        case Type::Comparisons:
            return cs[ColorGroup::Operators];
        case Type::StrConstants:
        case Type::IntConstants:
        case Type::FPConstants:
        case Type::CharConstants:
            return cs[ColorGroup::Constants];

        case Type::Identifiers:
        case Type::Other:
        case Type::Virtual:
        case Type::NonInterchangeable:
            break;
    }

    return cs[ColorGroup::Other];
}

// Checks whether node spelling can be diffed.
static bool
isDiffable(const Node &node, State state)
{
    return node.relative != nullptr
        && (node.stype == SType::Comment || node.type == Type::StrConstants)
        && state == State::Updated;
}

/**
 * @brief Diffs two labels.
 *
 * @param l        Original label.
 * @param r        Updated label.
 * @param dec      Default decoration.
 * @param original Whether this is original source.
 *
 * @returns Differed label.
 */
static std::string
diffSpelling(const std::string &l, const std::string &r,
             const decor::Decoration &dec, bool original)
{
    // XXX: some kind of caching would be nice.

    using namespace decor;
    using namespace decor::literals;

    std::vector<boost::string_ref> lWords = toWords(l);
    std::vector<boost::string_ref> rWords = toWords(r);

    auto cmp = [](const boost::string_ref &a, const boost::string_ref &b) {
        return (a == b);
    };

    dtl::Diff<boost::string_ref, std::vector<boost::string_ref>,
              decltype(cmp)> diff(lWords, rWords, cmp);
    diff.compose();

    std::ostringstream oss;

    Decoration deleted = (210_fg + inv + black_bg + bold)
                         .prefix("{-"_lit)
                         .suffix("-}"_lit);
    Decoration inserted = (85_fg + inv + black_bg + bold)
                          .prefix("{+"_lit)
                          .suffix("+}"_lit);

    const char *lastL = l.data(), *lastR = r.data();

    // FIXME: could do a better job than colorizing each character.

    auto printLeft = [&](const dtl::elemInfo &info, const Decoration &dec) {
        const boost::string_ref sr = lWords[info.beforeIdx - 1];
        oss << boost::string_ref(lastL, sr.data() - lastL);
        lastL = sr.data() + sr.size();
        oss << (dec << sr);
    };
    auto printRight = [&](const dtl::elemInfo &info, const Decoration &dec) {
        const boost::string_ref sr = rWords[info.afterIdx - 1];
        oss << boost::string_ref(lastR, sr.data() - lastR);
        lastR = sr.data() + sr.size();
        oss << (dec << sr);
    };

    for (const auto &x : diff.getSes().getSequence()) {
        switch (x.second.type) {
            case dtl::SES_DELETE:
                if (original) {
                    printLeft(x.second, deleted);
                }
                break;
            case dtl::SES_ADD:
                if (!original) {
                    printRight(x.second, inserted);
                }
                break;
            case dtl::SES_COMMON:
                if (original) {
                    printLeft(x.second, dec);
                } else {
                    printRight(x.second, dec);
                }
                break;
        }
    }

    oss << (dec << (original ? lastL : lastR));

    return oss.str();
}

/**
 * @brief Breaks a string into words.
 *
 * @param s Initial multi-word string.
 *
 * @returns Collection of words.
 */
static std::vector<boost::string_ref>
toWords(const std::string &s)
{
    std::vector<boost::string_ref> words;
    boost::string_ref sr(s);

    bool inWord = false;
    std::size_t wordStart = 0U;
    for (std::size_t i = 0U; i <= s.size(); ++i) {
        const bool isWordChar = s[i] != '\0'
                             && !std::isspace(s[i], std::locale());
        if (isWordChar != inWord) {
            if (isWordChar) {
                wordStart = i;
            } else {
                words.emplace_back(sr.substr(wordStart, i - wordStart));
            }
        }
        inWord = isWordChar;
    }

    return words;
}

// Formats spelling of a node into a colored string.
static std::string
getSpelling(const Node &node, State state, const decor::Decoration &dec,
            bool original)
{
    if (!isDiffable(node, state)) {
        return node.spelling;
    }

    return original
         ? diffSpelling(node.spelling, node.relative->spelling, dec, original)
         : diffSpelling(node.relative->spelling, node.spelling, dec, original);
}
