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
#include "STree.hpp"
#include "TreeBuilder.hpp"
#include "compare.hpp"
#include "decoration.hpp"
#include "integration.hpp"
#include "parser.hpp"
#include "time.hpp"
#include "tree.hpp"
#include "types.hpp"

namespace po = boost::program_options;

struct Args
{
    std::vector<std::string> pos;
    bool highlightMode;
    bool debug;    //!< Whether grammar debugging is enabled.
    bool sdebug;   //!< Whether stree debugging is enabled.
    bool dumpTree; //!< Whether to dump trees.
    bool dryRun;
    bool color;
    bool coarse;   //!< Whether to build coarse-grained tree.
    bool timeReport;
    bool noRefine;
    bool gitDiff;
};

static boost::optional<Args> parseArgs(const std::vector<std::string> &argv);
static int run(const Args &args, TimeReport &tr);
static po::variables_map parseOptions(const std::vector<std::string> &args);
static boost::optional<Tree> buildTreeFromFile(const std::string &path,
                                               const Args &args);

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
    if (!ifile) {
        throw std::runtime_error("Can't open file: " + path);
    }

    std::ostringstream iss;
    iss << ifile.rdbuf();
    return iss.str();
}

int
main(int argc, char *argv[]) try
{
    Args args;
    if (boost::optional<Args> a = parseArgs({ argv + 1, argv + argc })) {
        args = *a;
    } else {
        return EXIT_FAILURE;
    }

    RedirectToPager redirectToPager;
    TimeReport tr;

    const int result = run(args, tr);

    if (args.timeReport) {
        std::cout << tr;
    }

    return result;
}
catch (const std::exception &e)
{
    std::cerr << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
}

static int
run(const Args &args, TimeReport &tr)
{
    if (args.color) {
        decor::enableDecorations();
    }

    Tree treeA, treeB;
    auto dumpTrees = [&args, &treeA, &treeB]() {
        if (!args.dumpTree) {
            return;
        }

        if (Node *root = treeA.getRoot()) {
            std::cout << "Old tree:\n";
            print(*root);
        }

        if (Node *root = treeB.getRoot()) {
            std::cout << "New tree:\n";
            print(*root);
        }
    };

    const std::string oldFile = (args.gitDiff ? args.pos[1] : args.pos[0]);
    if (boost::optional<Tree> tree = (tr.measure("parsing1"),
                                      buildTreeFromFile(oldFile, args))) {
        treeA = std::move(*tree);
    } else {
        return EXIT_FAILURE;
    }

    if (args.highlightMode) {
        dumpTrees();
        if (!args.dryRun) {
            std::cout << printSource(*treeA.getRoot()) << '\n';
        }
        return EXIT_SUCCESS;
    }

    const std::string newFile = (args.gitDiff ? args.pos[4] : args.pos[1]);
    if (boost::optional<Tree> tree = (tr.measure("parsing2"),
                                      buildTreeFromFile(newFile, args))) {
        treeB = std::move(*tree);
    } else {
        return EXIT_FAILURE;
    }

    if (args.dryRun) {
        dumpTrees();
        return EXIT_SUCCESS;
    }

    // markSatellites(*treeA.getRoot());
    // markSatellites(*treeB.getRoot());

    Node *T1 = treeA.getRoot(), *T2 = treeB.getRoot();
    compare(T1, T2, tr, args.coarse, args.noRefine);

    dumpTrees();

    Printer printer(*T1, *T2);
    if (args.gitDiff) {
        printer.addHeader({ args.pos[3], args.pos[6] });
        printer.addHeader({ "a/" + args.pos[0], "b/" + args.pos[0] });
    } else {
        printer.addHeader({ oldFile, newFile });
    }
    tr.measure("printing"), printer.print();

    // printTree("T1", *T1);
    // printTree("T2", *T2);

    return EXIT_SUCCESS;
}

static boost::optional<Args>
parseArgs(const std::vector<std::string> &argv)
{
    po::variables_map varMap;
    try {
        varMap = parseOptions(argv);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return {};
    }

    Args args;

    args.pos = varMap["positional"].as<std::vector<std::string>>();
    if ((args.pos.empty() || args.pos.size() > 2U) && args.pos.size() != 7U) {
        std::cerr << "Wrong arguments\n";
        return {};
    }

    args.highlightMode = (args.pos.size() == 1U);
    args.debug = varMap.count("debug");
    args.sdebug = varMap.count("sdebug");
    args.dumpTree = varMap.count("dump-tree");
    args.dryRun = varMap.count("dry-run");
    args.color = varMap.count("color");
    args.coarse = varMap.count("coarse");
    args.timeReport = varMap.count("time-report");
    args.noRefine = varMap.count("no-refine");
    args.gitDiff = (args.pos.size() == 7U);

    return args;
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
        ("sdebug",       "enable debugging of strees")
        ("dump-tree",    "display tree(s)")
        ("coarse",       "use coarse-grained tree")
        ("time-report",  "report time spent on different activities")
        ("color",        "force colorization of output")
        ("no-refine",    "do not refine coarse results");

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
 * @param path  Path to the file to read.
 * @param args  Arguments of the application.
 *
 * @returns Tree on success or empty optional on error.
 */
static boost::optional<Tree>
buildTreeFromFile(const std::string &path, const Args &args)
{
    const std::string contents = readFile(path);

    TreeBuilder tb = parse(contents, path, args.debug);
    if (tb.hasFailed()) {
        return {};
    }

    Tree t;

    if (args.coarse) {
        STree stree(std::move(tb), contents, args.dumpTree, args.sdebug);
        t = Tree(contents, stree.getRoot());
    } else {
        t = Tree(contents, tb.getRoot());
    }

    return t;
}
