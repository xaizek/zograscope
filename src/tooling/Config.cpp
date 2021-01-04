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

namespace fs = boost::filesystem;

static bool pathIsInSubtree(const fs::path &root, const fs::path &path);
static fs::path normalizePath(const fs::path &path);
static fs::path makeRelativePath(fs::path base, fs::path path);
static bool isGlob(const std::string &expr);
static std::string globToRegex(boost::string_ref glob);

static const char ConfigDirName[] = ".zs";
static const char ExcludeFileName[] = "exclude";

// Type of exclude expression.
enum class ExcludeType
{
    Exact, // Exact match.
    Glob,  // Glob expression (implemented as a regexp).
};

// Single exclude expression.
class Config::ExcludeExpr
{
public:
    // Recognizes expression and prepares for matching against it.
    explicit ExcludeExpr(std::string expr);

public:
    // Checks for a match.
    bool matches(const std::string &relative,
                 const std::string &filename) const;

private:
    std::regex regexp; // Regular-expression match.
    std::string exact; // String to match against exactly.
    ExcludeType type;  // Type of the expression.
    bool filenameOnly; // Matches only against tail entry of a path.
};

Config::ExcludeExpr::ExcludeExpr(std::string expr)
{
    filenameOnly = (expr.find('/') == std::string::npos);

    // We don't need a special flag for this syntax, `filenameOnly` being
    // `false` is enough.
    bool rootOnly = (expr.front() == '/');
    if (rootOnly) {
        expr.erase(expr.begin());
    }

    if (isGlob(expr)) {
        regexp = globToRegex(expr);
        type = ExcludeType::Glob;
    } else {
        exact = std::move(expr);
        type = ExcludeType::Exact;
    }
}

bool
Config::ExcludeExpr::matches(const std::string &relative,
                             const std::string &filename) const
{
    const std::string &str = (filenameOnly ? filename : relative);

    switch (type) {
        case ExcludeType::Exact:
            return (str == exact);
        case ExcludeType::Glob:
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

    std::ifstream excludeFile(excludeFilePath.string());
    for (std::string line; std::getline(excludeFile, line); ) {
        if (!line.empty() && line.front() != '#') {
            excluded.emplace_back(normalizePath(line).string());
        }
    }
}

bool
Config::shouldProcessFile(const std::string &path) const
{
    fs::path canonicPath = normalizePath(fs::absolute(path, currDir));

    if (!pathIsInSubtree(rootDir, canonicPath)) {
        return true;
    }

    fs::path relPath = makeRelativePath(rootDir, canonicPath);
    std::string relative = relPath.string();
    std::string filename = relPath.filename().string();
    for (const ExcludeExpr &expr : excluded) {
        if (expr.matches(relative, filename)) {
            return false;
        }
    }
    return true;
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

    std::string regexp = "^";

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

    regexp += '$';
    return regexp;
}
