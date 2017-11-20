#ifndef COMMONARGS_HPP__
#define COMMONARGS_HPP__

#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

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

boost::program_options::variables_map
parseArgs(CommonArgs &args, const std::vector<std::string> &argv,
          const boost::program_options::options_description &options = {});

#endif // COMMONARGS_HPP__
