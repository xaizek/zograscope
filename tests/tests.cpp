#include "tests.hpp"

#include "Catch/catch.hpp"

#include <functional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/utility/string_ref.hpp>

#include "STree.hpp"
#include "compare.hpp"
#include "parser.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "utils.hpp"

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

static int countNodesInState(const Node &root, State state, bool leaves);
static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src);
static std::pair<std::string, std::string> splitAt(const boost::string_ref &s,
                                                   const std::string &delim);
static std::vector<Changes> makeChangeMap(Node &root);
static std::ostream & operator<<(std::ostream &os, Changes changes);

Tree
makeTree(const std::string &str, bool coarse)
{
    TreeBuilder tb = parse(str);
    REQUIRE_FALSE(tb.hasFailed());

    if (!coarse) {
        return Tree(str, tb.getRoot());
    }

    STree stree(std::move(tb), str);
    return Tree(str, stree.getRoot());
}

const Node *
findNode(const Tree &tree, Type type, const std::string &label)
{
    const Node *needle = nullptr;

    std::function<bool(const Node *)> visit = [&](const Node *node) {
        if (node->type == type) {
            if (label.empty() || node->label == label) {
                needle = node;
                return true;
            }
        }

        for (const Node *child : node->children) {
            if (visit(child)) {
                return true;
            }
        }

        return false;
    };

    visit(tree.getRoot());
    return needle;
}

int
countLeaves(const Node &root, State state)
{
    return countNodesInState(root, state, true);
}

int
countInternal(const Node &root, State state)
{
    return countNodesInState(root, state, false);
}

static int
countNodesInState(const Node &root, State state, bool leaves)
{
    int count = 0;

    std::function<void(const Node &)> visit = [&](const Node &node) {
        if (node.state == State::Unchanged && node.next != nullptr) {
            return visit(*node.next);
        }

        if (node.children.empty() == leaves && node.state == state) {
            ++count;
        }

        for (const Node *child : node.children) {
            visit(*child);
        }
    };

    visit(root);
    return count;
}

void
diffSources(const std::string &left, const std::string &right, bool skipRefine)
{
    std::string cleanedLeft, cleanedRight;
    std::vector<Changes> expectedOld, expectedNew;
    std::tie(cleanedLeft, expectedOld) = extractExpectations(left);
    std::tie(cleanedRight, expectedNew) = extractExpectations(right);

    Tree oldTree = makeTree(cleanedLeft, true);
    Tree newTree = makeTree(cleanedRight, true);

    TimeReport tr;
    compare(oldTree.getRoot(), newTree.getRoot(), tr, true, skipRefine);

    std::vector<Changes> oldMap = makeChangeMap(*oldTree.getRoot());
    std::vector<Changes> newMap = makeChangeMap(*newTree.getRoot());
    CHECK(oldMap == expectedOld);
    CHECK(newMap == expectedNew);
}

static std::pair<std::string, std::vector<Changes>>
extractExpectations(const std::string &src)
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
        std::tie(src, expectation) = splitAt(line, "/// ");

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
            REQUIRE_FALSE(true);
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
makeChangeMap(Node &root)
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
                markTreeAsMoved(node.next);
            }
            return visit(*node.next);
        }

        if (node.line != 0 && node.col != 0) {
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
    visit(root);

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
