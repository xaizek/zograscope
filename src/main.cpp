#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Printer.hpp"
#include "TreeBuilder.hpp"
#include "compare.hpp"
#include "decoration.hpp"
#include "integration.hpp"
#include "parser.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "types.hpp"

namespace po = boost::program_options;

static po::variables_map parseOptions(const std::vector<std::string> &args);
static boost::optional<Tree> buildTreeFromFile(const std::string &path,
                                               bool coarse, bool dump,
                                               bool debug);

// TODO: try marking tokens with types and accounting for them on rename
// TODO: try using string edit distance on rename
// TODO: try using token streams to find edit distance
// TODO: try using token streams to construct common diff

// TODO: try changing tree structure to get better results (join block node with { and } nodes)
//       e.g. by not feeding terminal nodes of non-terminal nodes that have
//       non-terminal children to the differ and later propagating state of
//       parent nodes to such children
static void
markSatellites(Node &node)
{
    auto nonTerminal = [](const Node *node) {
        return node->line == 0 || node->col == 0 || !node->children.empty();
    };
    auto terminal = [&nonTerminal](const Node *node) {
        return !nonTerminal(node);
    };

    if (std::any_of(node.children.cbegin(), node.children.cend(), nonTerminal)
        || std::all_of(node.children.cbegin(), node.children.cend(), terminal)) {
        for (Node *child : node.children) {
            child->satellite = !nonTerminal(child);
            markSatellites(*child);
        }
    }
}

std::string
readFile(const std::string &path)
{
    std::ifstream ifile(path);
    std::ostringstream iss;
    iss << ifile.rdbuf();
    return iss.str();
}

int
main(int argc, char *argv[])
{
    po::variables_map varMap;
    try {
        varMap = parseOptions({ argv + 1, argv + argc });
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    const auto args = varMap["positional"].as<std::vector<std::string>>();
    if ((args.empty() || args.size() > 2U) && args.size() != 7U) {
        std::cerr << "Wrong arguments\n";
        return EXIT_FAILURE;
    }

    const bool highlightMode = (args.size() == 1U);
    const bool debug = varMap.count("debug");
    const bool dumpTree = varMap.count("dump-tree");
    const bool dryRun = varMap.count("dry-run");
    const bool color = varMap.count("color");
    const bool coarse = varMap.count("coarse");
    const bool timeReport = varMap.count("time-report");

    if (color) {
        decor::enableDecorations();
    }

    RedirectToPager redirectToPager;
    TimeReport tr;

    Tree treeA;
    const std::string oldFile = (args.size() == 7U ? args[1] : args[0]);
    if (boost::optional<Tree> tree = (tr.measure("parsing1"),
                                      buildTreeFromFile(oldFile, coarse,
                                                        dumpTree, debug))) {
        treeA = std::move(*tree);
    } else {
        return EXIT_FAILURE;
    }

    if (highlightMode) {
        if (dryRun) {
            std::cout << ">>> Skipping coloring\n";
        } else {
            std::cout << printSource(*treeA.getRoot()) << '\n';
        }
        return EXIT_SUCCESS;
    }

    Tree treeB;
    const std::string newFile = (args.size() == 7U ? args[4] : args[1]);
    if (boost::optional<Tree> tree = (tr.measure("parsing2"),
                                      buildTreeFromFile(newFile, coarse,
                                                        dumpTree, debug))) {
        treeB = std::move(*tree);
    } else {
        return EXIT_FAILURE;
    }

    if (dryRun) {
        std::cout << ">>> Skipping diffing\n";
        return EXIT_SUCCESS;
    }

    // markSatellites(*treeA.getRoot());
    // markSatellites(*treeB.getRoot());

    Node *T1 = treeA.getRoot(), *T2 = treeB.getRoot();
    compare(T1, T2, tr, coarse);

    Printer printer(*T1, *T2);
    tr.measure("printing"), printer.print();

    // printTree("T1", *T1);
    // printTree("T2", *T2);

    if (timeReport) {
        std::cout << tr;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Parses command line-options.
 *
 * Positional arguments are returned in "positional" entry, which exists even
 * when there is no positional arguments.
 *
 * @param args Command-line arguments.
 *
 * @returns Variables map of option values.
 */
static po::variables_map
parseOptions(const std::vector<std::string> &args)
{
    po::options_description hiddenOpts;
    hiddenOpts.add_options()
        ("positional", po::value<std::vector<std::string>>()
                       ->default_value({}, ""),
         "positional args");

    po::positional_options_description positionalOptions;
    positionalOptions.add("positional", -1);

    po::options_description cmdlineOptions;

    cmdlineOptions.add_options()
        ("dry-run",      "just parse")
        ("debug",        "enable debugging of grammar")
        ("dump-tree",    "display tree(s)")
        ("coarse",       "use coarse-grained tree")
        ("time-report",  "report time spent on different activities")
        ("color",        "force colorization of output");

    po::options_description allOptions;
    allOptions.add(cmdlineOptions).add(hiddenOpts);

    auto parsed_from_cmdline =
        po::command_line_parser(args)
        .options(allOptions)
        .positional(positionalOptions)
        .run();

    po::variables_map varMap;
    po::store(parsed_from_cmdline, varMap);
    return varMap;
}

/**
 * @brief Reads and parses a file to build its tree.
 *
 * @param path     Path to the file to read.
 * @param coarse   Whether to build coarse-grained tree.
 * @param dump     Whether to dump trees.
 * @param debug    Whether grammar debugging is enabled.
 *
 * @returns Tree on success or empty optional on error.
 */
static boost::optional<Tree>
buildTreeFromFile(const std::string &path, bool coarse, bool dump, bool debug)
{
    std::cout << ">>> Parsing " << path << '\n';

    const std::string contents = readFile(path);

    TreeBuilder tb = parse(contents, debug);
    if (tb.hasFailed()) {
        return {};
    }

    Tree t = coarse ? Tree(contents, tb.makeSTree(contents, dump))
                    : Tree(contents, tb.getRoot());

    if (dump) {
        std::cout << "Tree of " << path << ":\n";
        print(*t.getRoot());
    }

    return t;
}
