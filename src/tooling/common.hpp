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

#ifndef ZOGRASCOPE__TOOLING__COMMON_HPP__
#define ZOGRASCOPE__TOOLING__COMMON_HPP__

#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "integration.hpp"

class Tree;

namespace cpp17 {
    namespace pmr {
        class memory_resource;
    }
}

// Common set of command-line arguments.
struct CommonArgs
{
    std::vector<std::string> pos; // Positional arguments.
    std::string lang;             // Forced language.
    bool help;                    // Print help message.
    bool debug;                   // Whether grammar debugging is enabled.
    bool sdebug;                  // Whether stree debugging is enabled.
    bool dumpSTree;               // Whether to dump strees.
    bool dumpTree;                // Whether to dump trees.
    bool dryRun;                  // Exit after parsing.
    bool color;                   // Fine-grained tree.
    bool fine;                    // Whether to build only fine-grained tree.
    bool timeReport;              // Print time report.
};

class Environment
{
    using options_description = boost::program_options::options_description;
    using variables_map = boost::program_options::variables_map;

public:
    explicit Environment(const options_description &extraOpts = {})
        : options(extraOpts)
    {
    }

public:
    void setup(const std::vector<std::string> &argv);

    void teardown(bool error = false);

    void printOptions();

    TimeReport & getTimeKeeper()
    {
        return tr;
    }

    const CommonArgs & getCommonArgs() const
    {
        return args;
    }

    const variables_map & getVarMap() const
    {
        return varMap;
    }

private:
    options_description options;
    variables_map varMap;
    CommonArgs args;

    RedirectToPager redirectToPager;
    TimeReport tr;
};

// Reads and parses a file to build its tree.
optional_t<Tree> buildTreeFromFile(const std::string &path,
                                   const CommonArgs &args, TimeReport &tr,
                                   cpp17::pmr::memory_resource *mr);

void dumpTree(const CommonArgs &args, Tree &tree);

void dumpTrees(const CommonArgs &args, Tree &treeA, Tree &treeB);

#endif // ZOGRASCOPE__TOOLING__COMMON_HPP__
