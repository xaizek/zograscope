// Copyright (C) 2021 xaizek <xaizek@posteo.net>
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

#include "Config.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/utility/string_ref.hpp>

#include <cassert>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <utility>

#include "utils/strings.hpp"

namespace fs = boost::filesystem;

static bool pathIsInSubtree(const fs::path &root, const fs::path &path);
static fs::path normalizePath(const fs::path &path);
static fs::path makeRelativePath(fs::path base, fs::path path);
static bool isGlob(const std::string &expr);
static std::string globToRegex(boost::string_ref glob);
static Attrs extractAttrs(const std::vector<boost::string_ref> &parts);
static Attrs & operator+=(Attrs &lhs, const Attrs &rhs);

static const char ConfigDirName[] = ".zs";
static const char ExcludeFileName[] = "exclude";
static const char AttributesFileName[] = "attributes";
static const char TabWidthAttr[] = "tab-size";
static const char LangAttr[] = "lang";

// Type of match expression.
enum class MatchType
{
    Exact, // Exact match.
    Glob,  // Glob expression (implemented as a regexp).
};

// Single match expression.
class Config::MatchExpr
{
public:
    // Recognizes expression and prepares for matching against it.
    MatchExpr(std::string expr, bool directoryOnly);

public:
    // Checks for a match.
    bool matches(const std::string &relative,
                 const std::string &filename,
                 bool isDir) const;
    // Checks whether this is an exception from other rules.
    bool isException() const
    { return exception; }

private:
    std::regex regexp;  // Regular-expression match.
    std::string exact;  // String to match against exactly.
    MatchType type;     // Type of the expression.
    bool filenameOnly;  // Matches only against tail entry of a path.
    bool directoryOnly; // Matches only directories.
    bool exception;     // Whether this rule defines exception.
};

Config::MatchExpr::MatchExpr(std::string expr, bool directoryOnly) :
    directoryOnly(directoryOnly)
{
    filenameOnly = (expr.find('/') == std::string::npos);

    exception = (expr.front() == '!');
    if (exception) {
        expr.erase(expr.begin());
    }

    // We don't need a special flag for this syntax, `filenameOnly` being
    // `false` is enough.
    bool rootOnly = (expr.front() == '/');
    if (rootOnly) {
        expr.erase(expr.begin());
    }

    if (isGlob(expr)) {
        regexp = globToRegex(expr);
        type = MatchType::Glob;
    } else {
        exact = std::move(expr);
        type = MatchType::Exact;
    }
}

bool
Config::MatchExpr::matches(const std::string &relative,
                           const std::string &filename,
                           bool isDir) const
{
    if (directoryOnly && !isDir) {
        return false;
    }

    const std::string &str = (filenameOnly ? filename : relative);

    switch (type) {
        case MatchType::Exact:
            return (str == exact);
        case MatchType::Glob:
            return std::regex_match(str.begin(), str.end(), regexp);
    }

    assert(false && "Unhandled type.");
    return false;
}

Config::Config(const boost::filesystem::path &currDir) : currDir(currDir)
{
    fs::path configDir = discoverRoot();
    if (!configDir.empty()) {
        loadConfigDir(configDir);
    }
}

Config::~Config() = default;

fs::path
Config::discoverRoot()
{
    fs::path rootCandidate = currDir;
    while (!rootCandidate.empty()) {
        fs::path configDirCandidate = rootCandidate / ConfigDirName;
        if (fs::is_directory(configDirCandidate)) {
            rootDir = rootCandidate;
            return configDirCandidate;
        }
        rootCandidate = rootCandidate.parent_path();
    }
    return fs::path();
}

void
Config::loadConfigDir(const fs::path &configDir)
{
    fs::path excludeFilePath = configDir / ExcludeFileName;
    fs::path attributesFilePath = configDir / AttributesFileName;

    std::ifstream excludeFile(excludeFilePath.string());
    for (std::string line; std::getline(excludeFile, line); ) {
        if (!line.empty() && line.front() != '#') {
            bool directoryOnly = (line.back() == '/');
            excludeRules.emplace_back(normalizePath(line).string(),
                                      directoryOnly);
        }
    }

    std::ifstream attrsFile(attributesFilePath.string());
    for (std::string line; std::getline(attrsFile, line); ) {
        boost::trim_if(line, boost::is_any_of("\r\n \t"));
        if (!line.empty() && line.front() != '#') {
            std::vector<boost::string_ref> parts = split(line, ' ');

            MatchExpr expr(normalizePath(parts[0].to_string()).string(),
                           /*directoryOnly=*/false);
            if (!expr.isException()) {
                attrRules.emplace_back(std::move(expr), extractAttrs(parts));
            }
        }
    }
}

bool
Config::shouldVisitDirectory(const std::string &path) const
{
    return isAllowed(path, /*isDir=*/true);
}

bool
Config::shouldProcessFile(const std::string &path) const
{
    return isAllowed(path, /*isDir=*/false);
}

bool
Config::isAllowed(const std::string &path, bool isDir) const
{
    fs::path canonicPath = normalizePath(fs::absolute(path, currDir));

    if (!pathIsInSubtree(rootDir, canonicPath)) {
        return true;
    }

    fs::path relPath = makeRelativePath(rootDir, canonicPath);
    std::string relative = relPath.string();
    std::string filename = relPath.filename().string();

    auto it = std::find_if(excludeRules.cbegin(), excludeRules.cend(),
                           [&] (const MatchExpr &expr) {
        return !expr.isException() && expr.matches(relative, filename, isDir);
    });
    if (it == excludeRules.cend()) {
        return true;
    }

    it = std::find_if(it, excludeRules.cend(), [&] (const MatchExpr &expr) {
        return expr.isException() && expr.matches(relative, filename, isDir);
    });
    return (it != excludeRules.cend());
}

Attrs
Config::lookupAttrs(const std::string &path) const
{
    Attrs result;
    result.tabWidth = 4;

    fs::path canonicPath = normalizePath(fs::absolute(path, currDir));

    if (!pathIsInSubtree(rootDir, canonicPath)) {
        return result;
    }

    fs::path relPath = makeRelativePath(rootDir, canonicPath);
    std::string relative = relPath.string();
    std::string filename = relPath.filename().string();

    for (const auto &entry : attrRules) {
        if (entry.first.matches(relative, filename, /*isDir=*/false)) {
            result += entry.second;
        }
    }

    return result;
}

// Checks that a path is somewhere under specified root (root is considered to
// include itself).
static bool
pathIsInSubtree(const fs::path &root, const fs::path &path)
{
    auto rootLen = std::distance(root.begin(), root.end());
    auto pathLen = std::distance(path.begin(), path.end());
    if (pathLen < rootLen) {
        return false;
    }

    return std::equal(root.begin(), root.end(), path.begin());
}

// Excludes `..` and `.` entries from a path.
static fs::path
normalizePath(const fs::path &path)
{
    fs::path result;
    for (fs::path::iterator it = path.begin(); it != path.end(); ++it) {
        if (*it == "..") {
            if(result.filename() == "..") {
                result /= *it;
            } else {
                result = result.parent_path();
            }
        } else if (*it != ".") {
            result /= *it;
        }
    }
    return result;
}

// Makes path relative to specified base directory.
static fs::path
makeRelativePath(fs::path base, fs::path path)
{
    auto baseIt = base.begin();
    auto pathIt = path.begin();

    while (baseIt != base.end() && pathIt != path.end() && *pathIt == *baseIt) {
        ++pathIt;
        ++baseIt;
    }

    fs::path finalPath;
    while (baseIt != base.end()) {
        finalPath /= "..";
        ++baseIt;
    }

    while (pathIt != path.end()) {
        finalPath /= *pathIt;
        ++pathIt;
    }

    return finalPath;
}

// Checks whether expression is a glob.
static bool
isGlob(const std::string &expr)
{
    return (expr.find_first_of("[?*") != std::string::npos);
}

// Converts the glob into equivalent regular expression.
static std::string
globToRegex(boost::string_ref glob)
{
    boost::string_ref charsToEscape = "^.$()|+{";

    std::string regexp;

    auto toChar = [&regexp, &glob](char ch) {
        regexp += ch;
        glob.remove_prefix(1U);
    };
    auto toStr = [&regexp, &glob](const char str[]) {
        regexp += str;
        glob.remove_prefix(1U);
    };

    bool startedCharClass = false;

    while (!glob.empty()) {
        bool newCharClass = startedCharClass;
        startedCharClass = false;

        char ch = glob.front();
        if (newCharClass && (ch == '!' || ch == '^')) {
            toChar('^');
        } else if (ch == '\\') {
            toChar('\\');
            if (!glob.empty()) {
                toChar(glob.front());
            }
        } else if (ch == '?') {
            toStr("[^/]");
        } else if (ch == '*') {
            toStr("[^/]*");
        } else {
            startedCharClass = (ch == '[');
            if (charsToEscape.find(ch) != boost::string_ref::npos) {
                regexp += '\\';
            }
            toChar(ch);
        }
    }

    return regexp;
}

// Extracts attributes from a line from an attribute file.
static Attrs
extractAttrs(const std::vector<boost::string_ref> &parts)
{
    Attrs attrRules;

    for (auto it = ++parts.begin(); it < parts.end(); ++it) {
        std::string name, value;
        std::tie(name, value) = splitAt(it->to_string(), '=');

        if (name == TabWidthAttr) {
            std::size_t pos;
            int width = std::stoi(value, &pos);
            if (pos == value.length() && width > 0) {
                attrRules.tabWidth = width;
            }
        } else if (name == LangAttr) {
            attrRules.lang = value;
        }
    }

    return attrRules;
}

// Merges two sets of attributes together.
static Attrs &
operator+=(Attrs &lhs, const Attrs &rhs)
{
    if (!rhs.lang.empty()) {
        lhs.lang = rhs.lang;
    }
    if (rhs.tabWidth > 0) {
        lhs.tabWidth = rhs.tabWidth;
    }
    return lhs;
}
