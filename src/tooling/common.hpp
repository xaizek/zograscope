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

#ifndef ZOGRASCOPE__TOOLING__COMMON_HPP__
#define ZOGRASCOPE__TOOLING__COMMON_HPP__

#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "Config.hpp"
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
    bool noPager;                 // Don't spawn a pager.
};

class Environment
{
    using options_description = boost::program_options::options_description;
    using variables_map = boost::program_options::variables_map;

public:
    explicit Environment(const options_description &extraOpts = {});

public:
    void setup(const std::vector<std::string> &argv);

    void teardown(bool error = false);

    void printOptions();

    TimeReport & getTimeKeeper()
    { return tr; }

    Config & getConfig()
    { return config; }

    const CommonArgs & getCommonArgs() const
    { return args; }

    const variables_map & getVarMap() const
    { return varMap; }

private:
    options_description options;
    variables_map varMap;
    CommonArgs args;

    RedirectToPager redirectToPager;
    TimeReport tr;
    Config config;
};

// Reads and parses a file to build its tree.
optional_t<Tree> buildTreeFromFile(Environment &env,
                                   const std::string &path,
                                   cpp17::pmr::memory_resource *mr);

// Reads and parses a file to build its tree.  This form allows specifying
// custom time keeper, which might be necessary for non-main threads.  It also
// accepts custom attributes to apply attributes of one file on another.
optional_t<Tree> buildTreeFromFile(Environment &env,
                                   TimeReport &tr,
                                   const Attrs &attrs,
                                   const std::string &path,
                                   cpp17::pmr::memory_resource *mr);

// As above, but with contents.
optional_t<Tree> buildTreeFromFile(Environment &env,
                                   TimeReport &tr,
                                   const Attrs &attrs,
                                   const std::string &path,
                                   const std::string &contents,
                                   cpp17::pmr::memory_resource *mr);

// Parses a file to build its tree.
optional_t<Tree> buildTreeFromFile(Environment &env,
                                   const std::string &path,
                                   const std::string &contents,
                                   cpp17::pmr::memory_resource *mr);

void dumpTree(const CommonArgs &args, Tree &tree);

void dumpTrees(const CommonArgs &args, Tree &treeA, Tree &treeB);

#endif // ZOGRASCOPE__TOOLING__COMMON_HPP__
