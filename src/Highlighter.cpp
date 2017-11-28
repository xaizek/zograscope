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

static void markTreeWithState(Node *node, State state);
static const decor::Decoration & getHighlight(const Node &node);
static bool isDiffable(const Node &node);
static std::string diffSpelling(const std::string &l, const std::string &r,
                                const decor::Decoration &dec, bool original);
static std::vector<boost::string_ref> toWords(const std::string &s);
static std::string getSpelling(const Node &node, const decor::Decoration &dec,
                               bool original);

class Highlighter::ColorPicker
{
public:
    void setNode(const Node &node)
    {
        prevNode = (currNode == &node ? nullptr : currNode);
        currNode = &node;

        prevHighlight = currHighlight;
        currHighlight = &::getHighlight(*currNode);
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
    : line(1), col(1), colorPicker(new ColorPicker()), original(original)
{
    toProcess.push(&root);
}

Highlighter::~Highlighter()
{
    // Emit destructor here with ColorPicker object being complete.
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
        Node *const node = getNode();
        if (!node->leaf) {
            advanceNode(node);
            continue;
        }

        colorPicker->setNode(*node);

        while (node->line > line) {
            if (++line == targetLine) {
                return;
            }
        }

        advanceNode(node);

        split(node->spelling, '\n', olines);

        for (std::size_t i = 1U; i < olines.size(); ++i) {
            if (++line == targetLine) {
                colorPicker->advancedLine();
                const decor::Decoration &dec = colorPicker->getHighlight();
                spelling = getSpelling(*node, dec, original);
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
        Node *const node = getNode();
        if (!node->leaf) {
            advanceNode(node);
            continue;
        }

        colorPicker->setNode(*node);

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

        advanceNode(node);

        spelling = getSpelling(*node, colorPicker->getHighlight(), original);
        split(node->spelling, '\n', olines);
        split(spelling, '\n', lines);

        printSpelling(n);
    }
}

void
Highlighter::printSpelling(int &n)
{
    const decor::Decoration &dec = colorPicker->getHighlight();
    oss << (dec << lines.front());
    col += olines.front().size();

    for (std::size_t i = 1U; i < lines.size(); ++i) {
        ++line;
        colorPicker->advancedLine();
        if (--n == 0) {
            olines.erase(olines.begin(), olines.begin() + i);
            lines.erase(lines.begin(), lines.begin() + i);
            return;
        }

        std::size_t whitespaceLength = lines[i].find_first_not_of(" \t");
        if (whitespaceLength == std::string::npos) {
            whitespaceLength = lines[i].size();
        }
        oss << '\n' << lines[i].substr(0, whitespaceLength)
                    << (dec << lines[i].substr(whitespaceLength));
        col = 1 + olines[i].size();
    }

    olines.clear();
    lines.clear();
}

Node *
Highlighter::getNode()
{
    Node *node = toProcess.top();

    if (node->next != nullptr) {
        if (node->state != State::Unchanged) {
            markTreeWithState(node->next, node->state);
        }
        if (node->moved) {
            markTreeAsMoved(node->next);
        }
        node = node->next;
    }

    return node;
}

void
Highlighter::advanceNode(Node *node)
{
    toProcess.pop();
    for (Node *child : boost::adaptors::reverse(node->children)) {
        toProcess.push(child);
    }
}

// Marks one layer of a tree specified by the node with the given state.
static void
markTreeWithState(Node *node, State state)
{
    node->state = state;
    for (Node *child : node->children) {
        markTreeWithState(child, state);
    }
}

/**
 * @brief Retrieves decoration for the specified node.
 *
 * @param node Node whose decoration is being queried.
 *
 * @returns The decoration.
 */
static const decor::Decoration &
getHighlight(const Node &node)
{
    static ColorScheme cs;

    // Highlighting based on node state has higher priority.
    switch (node.state) {
        case State::Deleted:
            return cs[ColorGroup::Deleted];
        case State::Inserted:
            return cs[ColorGroup::Inserted];
        case State::Updated:
            // TODO: things that were updated and moved at the same time need a
            //       special color?
            //       Or update kinda implies move, since it's obvious that there
            //       was a match between nodes?
            if (!isDiffable(node)) {
                return cs[ColorGroup::Updated];
            }
            break;

        case State::Unchanged:
            if (node.moved) {
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

/**
 * @brief Checks whether node spelling can be diffed.
 *
 * @param node Node to check.
 *
 * @returns @c true if so, @c false otherwise.
 */
static bool
isDiffable(const Node &node)
{
    return node.relative != nullptr
        && (node.stype == SType::Comment || node.type == Type::StrConstants)
        && node.state == State::Updated;
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

/**
 * @brief Formats spelling for a node.
 *
 * @param node     Source of the spelling.
 * @param dec      Default decoration.
 * @param original Whether this is original source.
 *
 * @returns Formatted spelling.
 */
static std::string
getSpelling(const Node &node, const decor::Decoration &dec, bool original)
{
    if (!isDiffable(node)) {
        return node.spelling;
    }

    return original
         ? diffSpelling(node.spelling, node.relative->spelling, dec, original)
         : diffSpelling(node.relative->spelling, node.spelling, dec, original);
}
