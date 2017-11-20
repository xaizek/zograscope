#include "CommonArgs.hpp"

#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

static po::variables_map parseOptions(const std::vector<std::string> &args,
                                      po::options_description options);

po::variables_map
parseArgs(CommonArgs &args, const std::vector<std::string> &argv,
          const po::options_description &options)
{
    po::variables_map varMap = parseOptions(argv, options);

    args.pos = varMap["positional"].as<std::vector<std::string>>();
    args.debug = varMap.count("debug");
    args.sdebug = varMap.count("sdebug");
    args.dumpSTree = varMap.count("dump-stree");
    args.dumpTree = varMap.count("dump-tree");
    args.dryRun = varMap.count("dry-run");
    args.color = varMap.count("color");
    args.fine = varMap.count("fine-only");
    args.timeReport = varMap.count("time-report");

    return varMap;
}

// Parses command line-options.
//
// Positional arguments are returned in "positional" entry, which exists even
// when there is no positional arguments.
static po::variables_map
parseOptions(const std::vector<std::string> &args,
             po::options_description options)
{
    po::options_description hiddenOpts;
    hiddenOpts.add_options()
        ("positional", po::value<std::vector<std::string>>()
                       ->default_value({}, ""),
         "positional args");

    po::positional_options_description positionalOptions;
    positionalOptions.add("positional", -1);

    options.add_options()
        ("dry-run",     "just parse")
        ("debug",       "enable debugging of grammar")
        ("sdebug",      "enable debugging of strees")
        ("dump-stree",  "display stree(s)")
        ("dump-tree",   "display tree(s)")
        ("fine-only",   "use only fine-grained tree")
        ("time-report", "report time spent on different activities")
        ("color",       "force colorization of output");

    po::options_description allOptions;
    allOptions.add(options).add(hiddenOpts);

    auto parsed_from_cmdline =
        po::command_line_parser(args)
        .options(allOptions)
        .positional(positionalOptions)
        .run();

    po::variables_map varMap;
    po::store(parsed_from_cmdline, varMap);
    return varMap;
}
