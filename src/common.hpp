#ifndef COMMON_HPP__
#define COMMON_HPP__

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
        : extraOpts(extraOpts)
    {
    }

public:
    void setup(const std::vector<std::string> &argv);

    void teardown(bool error = false);

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
    options_description extraOpts;
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

#endif // COMMON_HPP__
