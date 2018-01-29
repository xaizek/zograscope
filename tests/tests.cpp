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

#include "tests.hpp"

#include "Catch/catch.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/utility/string_ref.hpp>
#include "pmr/monolithic.hpp"

#include "utils/strings.hpp"
#include "utils/time.hpp"
#include "Language.hpp"
#include "Printer.hpp"
#include "STree.hpp"
#include "compare.hpp"
#include "decoration.hpp"
#include "tree.hpp"

enum class Changes
{
    No,
    Unchanged,
    Additions,
    Deletions,
    Updates,
    Moves,
    Mixed,
};

static bool isParsed(const std::string &fileName, const std::string &str);
static Tree parse(const std::string &fileName, const std::string &str,
                  bool coarse);
static void diffSources(const std::string &left, const std::string &right,
                        bool skipRefine, const std::string &fileName,
                        const std::string &marker);
static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src, const std::string &marker);
static std::pair<std::string, std::string> splitAt(const boost::string_ref &s,
                                                   const std::string &delim);
static std::vector<Changes> makeChangeMap(Tree &tree);
static std::ostream & operator<<(std::ostream &os, Changes changes);

bool
cIsParsed(const std::string &str)
{
    return isParsed("test-input.c", str);
}

bool
makeIsParsed(const std::string &str)
{
    return isParsed("Makefile", str);
}

// Checks whether source can be parsed or not.
static bool
isParsed(const std::string &fileName, const std::string &str)
{
    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create(fileName);
    return !lang->parse(str, "<input>", false, mr).hasFailed();
}

Tree
parseC(const std::string &str, bool coarse)
{
    return parse("test-input.c", str, coarse);
}

Tree
parseMake(const std::string &str)
{
    return parse("Makefile", str, true);
}

Tree
parseCxx(const std::string &str)
{
    return parse("test-input.cpp", str, true);
}

// Parses source into a tree.
static Tree
parse(const std::string &fileName, const std::string &str, bool coarse)
{
    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create(fileName);

    TreeBuilder tb = lang->parse(str, "<input>", false, mr);
    REQUIRE_FALSE(tb.hasFailed());

    if (!coarse) {
        return Tree(std::move(lang), str, tb.getRoot());
    }

    STree stree(std::move(tb), str, false, false, *lang, mr);
    return Tree(std::move(lang), str, stree.getRoot());
}

const Node *
findNode(const Tree &tree, Type type, const std::string &label)
{
    return findNode(tree, [&](const Node *node) {
                        if (node->type == type) {
                            if (label.empty() || node->label == label) {
                                return true;
                            }
                        }
                        return false;
                    });
}

const Node *
findNode(const Tree &tree, std::function<bool(const Node *)> pred)
{
    const Node *needle = nullptr;

    std::function<bool(const Node *)> visit = [&](const Node *node) {
        if (pred(node)) {
            needle = node;
            return true;
        }

        for (const Node *child : node->children) {
            if (visit(child)) {
                return true;
            }
        }

        if (node->next != nullptr) {
            return visit(node->next);
        }

        return false;
    };

    visit(tree.getRoot());
    return needle;
}

int
countLeaves(const Node &root, State state)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.state == State::Unchanged && node.next != nullptr) {
            return visit(*node.next);
        }

        if (node.children.empty() && node.state == state) {
            ++count;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };

    visit(root);
    return count;
}

int
countInternal(const Node &root, SType stype, State state)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.state == State::Unchanged && node.next != nullptr) {
            return visit(*node.next);
        }

        if (!node.children.empty() && node.stype == stype &&
            node.state == state) {
            ++count;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };

    visit(root);
    return count;
}

std::string
normalizeText(const std::string &s)
{
    std::string result;
    for (boost::string_ref sr : split(s, '\n')) {
        std::string line = sr.to_string();
        boost::trim_if(line, boost::is_any_of("\r\n \t"));

        if (!line.empty()) {
            result += line;
            result += '\n';
        }
    }
    return result;
}

void
diffC(const std::string &left, const std::string &right, bool skipRefine)
{
    diffSources(left, right, skipRefine, "test-input.c", "/// ");
}

void
diffMake(const std::string &left, const std::string &right)
{
    diffSources(left, right, true, "Makefile.test", "## ");
}

void
diffSrcmlCxx(const std::string &left, const std::string &right)
{
    diffSources(left, right, true, "test-input.cpp", "/// ");
}

// Compares two sources with expectation being embedded in them in form of
// trailing markers.
static void
diffSources(const std::string &left, const std::string &right, bool skipRefine,
            const std::string &fileName, const std::string &marker)
{
    std::string cleanedLeft, cleanedRight;
    std::vector<Changes> expectedOld, expectedNew;
    std::tie(cleanedLeft, expectedOld) = extractExpectations(left, marker);
    std::tie(cleanedRight, expectedNew) = extractExpectations(right, marker);

    Tree oldTree = parse(fileName, cleanedLeft, true);
    Tree newTree = parse(fileName, cleanedRight, true);

    TimeReport tr;
    compare(oldTree, newTree, tr, true, skipRefine);

    std::vector<Changes> oldMap = makeChangeMap(oldTree);
    std::vector<Changes> newMap = makeChangeMap(newTree);

    bool needPrint = false;
    CHECKED_ELSE(oldMap == expectedOld) {
        needPrint = true;
    }
    CHECKED_ELSE(newMap == expectedNew) {
        needPrint = true;
    }

    if (needPrint) {
        Tree oldTree = parse(fileName, left, true);
        Tree newTree = parse(fileName, right, true);

        compare(oldTree, newTree, tr, true, skipRefine);

        decor::enableDecorations();

        Printer printer(*oldTree.getRoot(), *newTree.getRoot(),
                        *oldTree.getLanguage(), std::cout);
        printer.addHeader({ "old", "new" });
        printer.print(tr);

        decor::disableDecorations();
    }
}

static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src, const std::string &marker)
{
    std::vector<boost::string_ref> lines = split(src, '\n');

    auto allSpaces = [](boost::string_ref str) {
        return (str.find_first_not_of(' ') == std::string::npos);
    };

    while (!lines.empty() && allSpaces(lines.back())) {
        lines.pop_back();
    }

    std::vector<Changes> changes;
    changes.reserve(lines.size());

    std::string cleanedSrc;
    cleanedSrc.reserve(src.length());

    for (boost::string_ref line : lines) {
        std::string src, expectation;
        std::tie(src, expectation) = splitAt(line, marker);

        // Some sources might be sensitive to trailing spaces (when newline is
        // escaped for example).
        boost::trim_right_if(src, boost::is_any_of(" \t"));

        cleanedSrc += src;
        cleanedSrc += '\n';

        if (expectation == "") {
            if (src.empty()) {
                changes.push_back(Changes::No);
            } else {
                changes.push_back(Changes::Unchanged);
            }
        } else if (expectation == "No") {
            changes.push_back(Changes::No);
        } else if (expectation == "Unchanged") {
            changes.push_back(Changes::Unchanged);
        } else if (expectation == "Additions") {
            changes.push_back(Changes::Additions);
        } else if (expectation == "Deletions") {
            changes.push_back(Changes::Deletions);
        } else if (expectation == "Updates") {
            changes.push_back(Changes::Updates);
        } else if (expectation == "Moves") {
            changes.push_back(Changes::Moves);
        } else if (expectation == "Mixed") {
            changes.push_back(Changes::Mixed);
        } else {
            CAPTURE(expectation);
            FAIL("Unknown expectation");
        }
    }

    return { cleanedSrc, changes };
}

/**
 * @brief Splits string in two parts at the leftmost delimiter.
 *
 * @param s String to split.
 * @param delim Delimiter, which separates left and right parts of the string.
 *
 * @returns Pair of left and right string parts.
 *
 * @throws std::runtime_error On failure to find delimiter in the string.
 */
static std::pair<std::string, std::string>
splitAt(const boost::string_ref &s, const std::string &delim)
{
    const std::string::size_type pos = s.find(delim);
    if (pos == std::string::npos) {
        return { s.to_string(), std::string() };
    }

    return { s.substr(0, pos).to_string(),
             s.substr(pos + delim.length()).to_string() };
}

static std::vector<Changes>
makeChangeMap(Tree &tree)
{
    std::vector<Changes> map;

    auto updateMap = [&](int line, const Node &node) {
        if (map.size() <= static_cast<unsigned int>(line)) {
            map.resize(line + 1);
        }

        Changes change = Changes::No;
        switch (node.state) {
            case State::Unchanged:
                change = (node.moved ? Changes::Moves : Changes::Unchanged);
                break;
            case State::Deleted:  change = Changes::Deletions; break;
            case State::Inserted: change = Changes::Additions; break;
            case State::Updated:  change = Changes::Updates; break;
        }

        if (map[line] == Changes::No) {
            map[line] = change;
        } else if (map[line] != change) {
            map[line] = Changes::Mixed;
        }
    };

    std::function<void(Node &, State)> mark = [&](Node &node, State state) {
        node.state = state;
        for (Node *child : node.children) {
            mark(*child, state);
        }
    };

    int line;
    std::function<void(Node &)> visit = [&](Node &node) {
        if (node.next != nullptr) {
            if (node.state != State::Unchanged) {
                mark(*node.next, node.state);
            }
            if (node.moved) {
                tree.markTreeAsMoved(node.next);
            }
            return visit(*node.next);
        }

        if (node.leaf) {
            line = node.line - 1;
            std::vector<boost::string_ref> lines = split(node.label, '\n');
            updateMap(line, node);
            for (std::size_t i = 1U; i < lines.size(); ++i) {
                updateMap(++line, node);
            }
        }

        for (Node *child : node.children) {
            visit(*child);
        }
    };
    visit(*tree.getRoot());

    return map;
}

static std::ostream &
operator<<(std::ostream &os, Changes changes)
{
    switch (changes) {
        case Changes::No:        return (os << "No");
        case Changes::Unchanged: return (os << "Unchanged");
        case Changes::Additions: return (os << "Additions");
        case Changes::Deletions: return (os << "Deletions");
        case Changes::Updates:   return (os << "Updates");
        case Changes::Moves:     return (os << "Moves");
        case Changes::Mixed:     return (os << "Mixed");
    }

    return (os << "Unknown Changes value");
}
