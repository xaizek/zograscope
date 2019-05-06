// Copyright (C) 2019 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__TOOLS__TUI__FILEREGISTRY_HPP__
#define ZOGRASCOPE__TOOLS__TUI__FILEREGISTRY_HPP__

#include <string>
#include <unordered_map>
#include <vector>

#include "pmr/monolithic.hpp"
#include "tree.hpp"

class CommonArgs;
class Node;
class TimeReport;

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
    FileRegistry(const CommonArgs &args, TimeReport &tr);

public:
    bool addFile(const std::string &path);

    const std::vector<FuncInfo> & getFuncInfos() const;
    const Tree & getTree(const std::string &path) const;

private:
    const CommonArgs &args;
    TimeReport &tr;

    cpp17::pmr::monolithic mr;
    std::unordered_map<std::string, Tree> trees;

    std::vector<FuncInfo> infos;
};

#endif // ZOGRASCOPE__TOOLS__TUI__FILEREGISTRY_HPP__
