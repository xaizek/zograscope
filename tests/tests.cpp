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

#include "tests.hpp"

#include "Catch/catch.hpp"

#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
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

namespace fs = boost::filesystem;

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
static std::string diffSources(const std::string &left,
                               const std::string &right,
                               bool skipRefine,
                               const std::string &fileName,
                               const std::string &marker);
static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src, const std::string &marker);
static std::pair<std::string, std::string> splitAt(const boost::string_ref &s,
                                                   const std::string &delim);
static std::vector<Changes> makeChangeMap(Tree &tree);
static std::vector<std::string> annotate(const std::vector<Changes> &expected,
                                         const std::vector<Changes> &actual);
static std::ostream & operator<<(std::ostream &os, Changes changes);

static const char AppPrefix[] = "zs-tests-";

TempDir::TempDir(const std::string &prefix)
{
    path = (
       fs::temp_directory_path()
     / fs::unique_path(AppPrefix + prefix + "-%%%%-%%%%")
    ).string();
    fs::create_directories(path);
}

TempDir::~TempDir()
{
    static_cast<void>(fs::remove_all(path));
}

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
    return !lang->parse(str, "<input>", /*tabWidth=*/4, false, mr).hasFailed();
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

Tree
parseLua(const std::string &str)
{
    return parse("test-input.lua", str, true);
}

// Parses source into a tree.
static Tree
parse(const std::string &fileName, const std::string &str, bool coarse)
{
    const int tabWidth = 4;

    cpp17::pmr::monolithic mr;
    std::unique_ptr<Language> lang = Language::create(fileName);

    TreeBuilder tb = lang->parse(str, "<input>", tabWidth, false, mr);
    REQUIRE_FALSE(tb.hasFailed());

    if (!coarse) {
        return Tree(std::move(lang), tabWidth, str, tb.getRoot());
    }

    STree stree(std::move(tb), str, false, false, *lang, mr);
    return Tree(std::move(lang), tabWidth, str, stree.getRoot());
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
findNode(const Tree &tree, std::function<bool(const Node *)> pred,
         bool skipLastLayer)
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

        if (node->next != nullptr && (!node->next->last || !skipLastLayer)) {
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
compareAndPrint(Tree &&original, Tree &&updated, bool skipRefine)
{
    TimeReport tr;
    compare(original, updated, tr, true, skipRefine);

    std::ostringstream oss;
    Printer printer(*original.getRoot(), *updated.getRoot(),
                    *original.getLanguage(), oss);
    printer.print(tr);

    return normalizeText(oss.str());
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

#undef diffC
std::string
diffC(const std::string &left, const std::string &right, bool skipRefine)
{
    return diffSources(left, right, skipRefine, "test-input.c", "/// ");
}

#undef diffMake
std::string
diffMake(const std::string &left, const std::string &right)
{
    return diffSources(left, right, true, "Makefile.test", "## ");
}

#undef diffSrcmlCxx
std::string
diffSrcmlCxx(const std::string &left, const std::string &right)
{
    return diffSources(left, right, true, "test-input.cpp", "/// ");
}

#undef diffTsLua
std::string
diffTsLua(const std::string &left, const std::string &right)
{
    return diffSources(left, right, true, "test-input.lua", "--- ");
}

#undef diffTsBash
std::string
diffTsBash(const std::string &left, const std::string &right)
{
    return diffSources(left, right, true, "test-input.sh", "### ");
}

// Compares two sources with expectation being embedded in them in form of
// trailing markers.  Returns difference report.
static std::string
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

    bool differ = (oldMap != expectedOld || newMap != expectedNew);

    if (differ) {
        Tree oldTree = parse(fileName, cleanedLeft, true);
        Tree newTree = parse(fileName, cleanedRight, true);

        compare(oldTree, newTree, tr, true, skipRefine);

        decor::enableDecorations();

        std::ostringstream oss;
        Printer printer(*oldTree.getRoot(), annotate(expectedOld, oldMap),
                        *newTree.getRoot(), annotate(expectedNew, newMap),
                        *oldTree.getLanguage(), oss);
        printer.addHeader({ "old", "new" });
        printer.print(tr);

        decor::disableDecorations();

        return oss.str();
    }

    return std::string();
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
    tree.propagateStates();

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

    int line;
    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.next != nullptr) {
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

        for (const Node *child : node.children) {
            visit(*child);
        }
    };
    visit(*tree.getRoot());

    return map;
}

// Generates per-line annotations for source code based on expected and actual
// changes.
static std::vector<std::string>
annotate(const std::vector<Changes> &expected,
         const std::vector<Changes> &actual)
{
    std::vector<std::string> annots;
    annots.reserve(expected.size());

    auto mismatch = decor::red_bg + decor::white_fg + decor::bold;
    auto match = decor::green_fg;

    std::ostringstream oss;
    for (std::size_t i = 0U; i < expected.size(); ++i) {
        bool matched = (actual[i] == expected[i]);
        if (!matched) {
            oss << mismatch;
        } else {
            oss << match;
        }
        oss << std::setw(9) << expected[i];
        oss << decor::def;
        oss << '|';
        annots.push_back(oss.str());
        oss.str(std::string());
    }

    return annots;
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

void
reportDiffFailure(const std::string &report)
{
    std::cout << report;
}

void
makeFile(const std::string &path, const std::vector<std::string> &lines)
{
    std::ofstream file(path);
    REQUIRE(file.is_open());
    for (const std::string &line : lines) {
        file << line << '\n';
    }
}
