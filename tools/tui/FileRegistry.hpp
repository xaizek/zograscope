// Copyright (C) 2019 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_TOOLS_TUI_FILEREGISTRY_HPP_
#define ZOGRASCOPE_TOOLS_TUI_FILEREGISTRY_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include "pmr/monolithic.hpp"
#include "tree.hpp"

class Environment;
class Node;

struct Location
{
    std::string path;
    int line;
    int col;
};

struct FuncInfo
{
    const Node *node;
    Location loc;
    int size;
    int params;
};

class FileRegistry
{
public:
    explicit FileRegistry(Environment &env);

public:
    bool addFile(const std::string &path);

    const std::vector<FuncInfo> & getFuncInfos() const;
    std::vector<std::string> listFileNames() const;
    const Tree & getTree(const std::string &path) const;

private:
    Environment &env;

    cpp17::pmr::monolithic mr;
    std::unordered_map<std::string, Tree> trees;

    std::vector<FuncInfo> infos;
};

#endif // ZOGRASCOPE_TOOLS_TUI_FILEREGISTRY_HPP_
