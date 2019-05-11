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

#include "FileRegistry.hpp"

#include <boost/optional.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "tooling/FunctionAnalyzer.hpp"
#include "tooling/common.hpp"
#include "utils/optional.hpp"
#include "NodeRange.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

FileRegistry::FileRegistry(const CommonArgs &args, TimeReport &tr)
    : args(args), tr(tr)
{ }

bool
FileRegistry::addFile(const std::string &path)
{
    Tree &tree = trees.emplace(path, Tree(&mr)).first->second;

    if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
        tree = *t;
    } else {
        std::cerr << "Failed to parse: " << path << '\n';
        return false;
    }

    dumpTree(args, tree);

    if (args.dryRun) {
        return true;
    }

    Language &lang = *tree.getLanguage();

    FunctionAnalyzer functionAnalyzer(lang);
    Location loc = { path, -1, -1 };
    for (const Node *node : NodeRange(tree.getRoot())) {
        if (!node->leaf && lang.classify(node->stype) == MType::Function) {
            loc.line = node->line;
            loc.col = node->col;

            int size = functionAnalyzer.getLineCount(node);
            int params = functionAnalyzer.getParamCount(node);
            infos.emplace_back(FuncInfo { node, loc, size, params });
        }
    }

    return true;
}

const std::vector<FuncInfo> &
FileRegistry::getFuncInfos() const
{
    return infos;
}

std::vector<std::string>
FileRegistry::listFileNames() const
{
    std::vector<std::string> files;
    files.reserve(trees.size());

    for (const auto &entry : trees) {
        files.push_back(entry.first);
    }

    return files;
}

const Tree &
FileRegistry::getTree(const std::string &path) const
{
    return trees.at(path);
}
