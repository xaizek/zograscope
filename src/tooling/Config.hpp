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

#ifndef ZOGRASCOPE__TOOLING__CONFIG_HPP__
#define ZOGRASCOPE__TOOLING__CONFIG_HPP__

#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

// Encapsulates configuration information.
class Config
{
    class ExcludeExpr;

public:
    // Discovers configuration files and root by visiting parents of the
    // specified path.
    explicit Config(const boost::filesystem::path &currDir);

    // Destroys `excluded` with full type.
    ~Config();

public:
    // Checks whether contents of specific directory should be looked at.
    bool shouldVisitDirectory(const std::string &path) const;
    // Checks whether specific file should be processed.
    bool shouldProcessFile(const std::string &path) const;

private:
    // Finds root directory (sets `rootDir`) and returns path to configuration
    // directory.  Both paths are empty if nothing was found.
    boost::filesystem::path discoverRoot();
    // Loads data from configuration directory.
    void loadConfigDir(const boost::filesystem::path &configDir);
    // Checks whether specific path should be processed.
    bool isAllowed(const std::string &path, bool isDir) const;

private:
    boost::filesystem::path rootDir; // Root for the configuration.
    boost::filesystem::path currDir; // Current directory for relative paths.

    std::vector<ExcludeExpr> excluded; // List of excluded file paths.
};

#endif // ZOGRASCOPE__TOOLING__CONFIG_HPP__
