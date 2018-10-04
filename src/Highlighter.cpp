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
#include "ColorCane.hpp"
#include "Language.hpp"
#include "colors.hpp"
#include "tree.hpp"

static int leftShift(const Node *node);
static ColorGroup getHighlight(const Node &node, int moved, State state,
                               const Language &lang);
static bool isDiffable(const Node &node, State state, const Language &lang);
static std::vector<boost::string_ref> toWords(const std::string &s);
static std::vector<boost::string_ref> toChars(const std::string &s);

// Single processing entry.
struct Highlighter::Entry
{
    const Node *node;    // Node to be processed.
    bool moved;          // Move status override for descendants.
    State state;         // State override for descendants.
    bool propagateMoved; // Moved field should be passed to all descendants.
    bool propagateState; // State field should be passed to all descendants.
};

class Highlighter::ColorPicker
{
public:
    ColorPicker(const Language &lang);

public:
    void setEntry(const Node *node, bool moved, State state);

    void advancedLine();

    ColorGroup getHighlight() const;
    ColorGroup getFillHighlight() const;

private:
    const Language &lang;
    ColorGroup currHighlight = ColorGroup::None;
    ColorGroup prevHighlight = ColorGroup::None;
    const Node *currNode = nullptr;
    const Node *prevNode = nullptr;
    bool prevMoved = false;
    bool currMoved = false;
};

Highlighter::ColorPicker::ColorPicker(const Language &lang) : lang(lang) { }

void
Highlighter::ColorPicker::setEntry(const Node *node, bool moved, State state)
{
    prevNode = (currNode == node ? nullptr : currNode);
    currNode = node;

    prevMoved = currMoved;
    currMoved = moved;

    prevHighlight = currHighlight;
    currHighlight = ::getHighlight(*currNode, moved, state, lang);
}

void
Highlighter::ColorPicker::advancedLine()
{
    prevNode = nullptr;
}

ColorGroup
Highlighter::ColorPicker::getHighlight() const
{
    return currHighlight;
}

ColorGroup
Highlighter::ColorPicker::getFillHighlight() const
{
    if (prevNode == nullptr) {
        return ColorGroup::None;
    }

    // Updates are individual (one to one) and look better separated with
    // background.
    if (prevHighlight == currHighlight &&
        currNode->state != State::Updated) {
        return currHighlight;
    }

    if (prevNode->leaf && prevMoved && currMoved) {
        return prevHighlight;
    }

    return ColorGroup::None;
}

Highlighter::Highlighter(const Tree &tree, bool original)
    : Highlighter(*tree.getRoot(), *tree.getLanguage(), original, 1, 1)
{
}

Highlighter::Highlighter(const Node &root, const Language &lang, bool original,
                         int lineOffset)
    : Highlighter(root, lang, original, lineOffset, leftShift(&root))
{
}

// Computes how far to the right subtree defined by the `node` is shifted.  This
// is the same amount by which it can be shifted to the left to get rid of
// unnecessary indentation.
static int
leftShift(const Node *node)
{
    auto getCol = [](const Node *node) {
        if (node->spelling.find('\n') != std::string::npos) {
            // Multiline nodes occupy first column.
            return 1;
        }
        return node->col;
    };

    if (node->next != nullptr) {
        return leftShift(node->next);
    }

    if (node->children.empty() && node->leaf) {
        return getCol(node);
    }

    int shift = std::numeric_limits<int>::max();
    for (Node *child : node->children) {
        if (!child->leaf || child->next != nullptr) {
            shift = std::min(shift, leftShift(child));
        } else {
            shift = std::min(shift, getCol(child));
        }
    }
    return shift;
}

Highlighter::Highlighter(const Node &root, const Language &lang, bool original,
                         int lineOffset, int colOffset)
    : lang(lang), line(lineOffset), col(1), colOffset(colOffset),
      colorPicker(new ColorPicker(lang)), original(original), current(nullptr),
      printReferences(false), printBrackets(true), transparentDiffables(true)
{
    toProcess.push({ &root, root.moved, root.state, false, false });
}

Highlighter::~Highlighter() = default;

void
Highlighter::setPrintReferences(bool print)
{
    printReferences = print;
}

void
Highlighter::setPrintBrackets(bool print)
{
    printBrackets = print;
}

void
Highlighter::setTransparentDiffables(bool transparent)
{
    transparentDiffables = transparent;
}

ColorCane
Highlighter::print(int from, int n)
{
    colorCane = ColorCane();
    if (from < line) {
        n = std::max(0, n - (line - from));
    }

    skipUntil(from);
    print(n);
    return std::move(colorCane);
}

ColorCane
Highlighter::print()
{
    colorCane = ColorCane();
    print(std::numeric_limits<int>::max());
    return std::move(colorCane);
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
        const Node *const node = entry.node;
        if (!node->leaf) {
            advance(entry);
            continue;
        }

        colorPicker->setEntry(entry.node, entry.moved, entry.state);

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
                ColorGroup def = transparentDiffables
                               ? ColorGroup::None
                               : ColorGroup::PieceUpdated;
                lines = getSpelling(*node, entry.state, def).splitIntoLines();
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
    col = colOffset;
    colorPicker->advancedLine();

    if (!lines.empty()) {
        printSpelling(n);
    }

    while (!toProcess.empty() && n != 0) {
        Entry entry = getEntry();
        const Node *const node = entry.node;
        if (!node->leaf) {
            advance(entry);
            continue;
        }

        colorPicker->setEntry(entry.node, entry.moved, entry.state);

        while (node->line > line) {
            ++line;
            colorPicker->advancedLine();
            if (--n == 0) {
                return;
            }
            colorCane.append('\n');
            col = colOffset;
        }

        if (node->col > col) {
            ColorGroup fillHi = colorPicker->getFillHighlight();
            colorCane.append(' ', node->col - col, fillHi);
            col = node->col;
        }

        advance(entry);

        ColorGroup def = transparentDiffables ? ColorGroup::None
                                              : ColorGroup::PieceUpdated;
        lines = getSpelling(*node, entry.state, def).splitIntoLines();
        split(node->spelling, '\n', olines);
        current = node;

        printSpelling(n);
    }
}

void
Highlighter::printSpelling(int &n)
{
    ColorGroup hi = colorPicker->getHighlight();

    auto printLine = [&](ColorCane &cc) {
        std::vector<ColorCane> ccs = std::move(cc).breakAt(" \t");

        for (auto &piece : ccs[0]) {
            colorCane.append(piece.text, piece.node, piece.hi);
        }
        for (auto &piece : ccs[1]) {
            ColorGroup cg = (piece.hi == ColorGroup::None ? hi : piece.hi);
            colorCane.append(piece.text, piece.node, cg);
        }
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

        colorCane.append('\n');
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
    const Node *node = entry.node;

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

// Determines color group for the specified node considering overrides of its
// properties.
static ColorGroup
getHighlight(const Node &node, int moved, State state, const Language &lang)
{
    // Highlighting based on node state has higher priority.
    switch (state) {
        case State::Deleted:
            return ColorGroup::Deleted;
        case State::Inserted:
            return ColorGroup::Inserted;
        case State::Updated:
            // TODO: things that were updated and moved at the same time need a
            //       special color?
            //       Or update kinda implies move, since it's obvious that there
            //       was a match between nodes?
            if (!isDiffable(node, state, lang)) {
                return ColorGroup::Updated;
            }
            break;

        case State::Unchanged:
            if (moved) {
                return ColorGroup::Moved;
            }
            break;
    }

    // Highlighting based on node type follows.
    switch (node.type) {
        case Type::Specifiers: return ColorGroup::Specifiers;
        case Type::UserTypes:  return ColorGroup::UserTypes;
        case Type::Types:      return ColorGroup::Types;
        case Type::Directives: return ColorGroup::Directives;
        case Type::Comments:   return ColorGroup::Comments;
        case Type::Functions:  return ColorGroup::Functions;

        case Type::Jumps:
        case Type::Keywords:
            return ColorGroup::Keywords;
        case Type::LeftBrackets:
        case Type::RightBrackets:
            return ColorGroup::Brackets;
        case Type::Assignments:
        case Type::Operators:
        case Type::LogicalOperators:
        case Type::Comparisons:
            return ColorGroup::Operators;
        case Type::StrConstants:
        case Type::IntConstants:
        case Type::FPConstants:
        case Type::CharConstants:
            return ColorGroup::Constants;

        case Type::Identifiers:
        case Type::Other:
        case Type::Virtual:
        case Type::NonInterchangeable:
            break;
    }

    return ColorGroup::Other;
}

ColorCane
Highlighter::getSpelling(const Node &node, State state, ColorGroup def)
{
    const bool diffable = isDiffable(node, state, lang);
    if (!diffable && state != State::Updated) {
        ColorCane cc;
        cc.append(node.spelling, &node);
        return cc;
    }

    ColorCane cc;
    if (diffable) {
        cc = diffSpelling(node, def);
    } else {
        cc.append(node.spelling, &node, ColorGroup::Updated);
    }

    int &id = updates[original ? &node : node.relative];
    id = updates.size();
    if (printReferences) {
        cc.append('{', ColorGroup::UpdatedSurroundings);
        cc.append(std::to_string(id), nullptr, ColorGroup::UpdatedSurroundings);
        cc.append('}', ColorGroup::UpdatedSurroundings);
    }

    return cc;
}

// Checks whether node spelling can be diffed.
static bool
isDiffable(const Node &node, State state, const Language &lang)
{
    return node.relative != nullptr
        && lang.isDiffable(&node)
        && state == State::Updated;
}

ColorCane
Highlighter::diffSpelling(const Node &node, ColorGroup def)
{
    // XXX: some kind of caching would be nice since we're doing the same thing
    //      for both original and updated nodes.

    const std::string &l = (original ? node.spelling : node.relative->spelling);
    const std::string &r = (original ? node.relative->spelling : node.spelling);

    std::vector<boost::string_ref> lWords = toWords(l);
    std::vector<boost::string_ref> rWords = toWords(r);

    const bool surround = node.type == Type::Functions
                       || node.type == Type::Identifiers
                       || node.type == Type::UserTypes;

    if (surround && lWords.size() == 1U && rWords.size() == 1U) {
        lWords = toChars(l);
        rWords = toChars(r);
    }

    auto cmp = [](const boost::string_ref &a, const boost::string_ref &b) {
        return (a == b);
    };

    dtl::Diff<boost::string_ref, std::vector<boost::string_ref>,
              decltype(cmp)> diff(lWords, rWords, cmp);
    diff.compose();

    ColorCane cc;

    float worstDistance = std::max(lWords.size(), rWords.size());
    float sim = 1.0f - diff.getEditDistance()/worstDistance;

    // If Levenshtein distance ends up being too big (similarity is too small),
    // drop comparison results and get back to just printing two nodes as
    // updated.
    if (sim < 0.2f) {
        cc.append(node.spelling, &node, ColorGroup::Updated);
        return cc;
    }

    if (surround && printBrackets) {
        cc.append('[', ColorGroup::UpdatedSurroundings);
    }

    const char *lastL = l.data(), *lastR = r.data();

    auto printLeft = [&](const dtl::elemInfo &info, ColorGroup hi,
                         ColorGroup def) {
        const boost::string_ref sr = lWords[info.beforeIdx - 1];
        cc.append(boost::string_ref(lastL, sr.data() - lastL), &node, def);
        cc.append(sr, &node, hi);
        lastL = sr.data() + sr.size();
    };
    auto printRight = [&](const dtl::elemInfo &info, ColorGroup hi,
                          ColorGroup def) {
        const boost::string_ref sr = rWords[info.afterIdx - 1];
        cc.append(boost::string_ref(lastR, sr.data() - lastR), &node, def);
        cc.append(sr, &node, hi);
        lastR = sr.data() + sr.size();
    };

    for (const auto &x : diff.getSes().getSequence()) {
        switch (x.second.type) {
            case dtl::SES_DELETE:
                if (original) {
                    printLeft(x.second, ColorGroup::PieceDeleted, def);
                }
                break;
            case dtl::SES_ADD:
                if (!original) {
                    printRight(x.second, ColorGroup::PieceInserted, def);
                }
                break;
            case dtl::SES_COMMON:
                if (original) {
                    printLeft(x.second, def, def);
                } else {
                    printRight(x.second, def, def);
                }
                break;
        }
    }

    cc.append(original ? lastL : lastR, &node, def);
    if (surround && printBrackets) {
        cc.append(']', ColorGroup::UpdatedSurroundings);
    }

    return cc;
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

    enum State { Start, WhiteSpace, Word, Punctuation, End };

    auto classify = [](char c) {
        if (c == '\0')                      return End;
        if (std::ispunct(c, std::locale())) return Punctuation;
        if (std::isspace(c, std::locale())) return WhiteSpace;
        return Word;
    };

    State currentState = Start;
    std::size_t wordStart = 0U;
    for (std::size_t i = 0U; i <= s.size(); ++i) {
        const State newState = classify(s[i]);
        // Each punctuation character is treated as a separate "word".
        if (currentState != newState || currentState == Punctuation) {
            if (currentState == Punctuation || currentState == Word) {
                words.emplace_back(sr.substr(wordStart, i - wordStart));
            }
            if (newState == Punctuation || newState == Word) {
                wordStart = i;
            }
        }
        currentState = newState;
    }

    return words;
}

static std::vector<boost::string_ref>
toChars(const std::string &s)
{
    std::vector<boost::string_ref> chars;
    boost::string_ref sr(s);

    for (std::size_t i = 0U; i < s.size(); ++i) {
        chars.emplace_back(sr.substr(i, 1));
    }

    return chars;
}
