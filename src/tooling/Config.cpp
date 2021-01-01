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

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = boost::filesystem;

static bool pathIsInSubtree(const fs::path &root, const fs::path &path);
static fs::path normalizePath(const fs::path &path);
static fs::path makeRelativePath(fs::path base, fs::path path);

static const char ConfigDirName[] = ".zs";
static const char ExcludeFileName[] = "exclude";

Config::Config(const boost::filesystem::path &currDir) : currDir(currDir)
{
    fs::path configDir = discoverRoot();
    if (!configDir.empty()) {
        loadConfigDir(configDir);
    }
}

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
            excluded.insert(normalizePath(line).string());
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
    return (excluded.find(relPath.string()) == excluded.end());
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
