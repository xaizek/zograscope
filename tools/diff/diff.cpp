#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pmr/monolithic.hpp"

#include "utils/optional.hpp"
#include "CommonArgs.hpp"
#include "Highlighter.hpp"
#include "Printer.hpp"
#include "common.hpp"
#include "compare.hpp"
#include "tree.hpp"

namespace po = boost::program_options;

struct Args : CommonArgs
{
    bool highlightMode;
    bool noRefine;
    bool gitDiff;
};

static Args parseLocArgs(const std::vector<std::string> &argv);
static int run(const Args &args, TimeReport &tr);

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

int
main(int argc, char *argv[])
{
    Args args;
    int result;

    try {
        args = parseLocArgs({ argv + 1, argv + argc });
        if ((args.pos.empty() || args.pos.size() > 2U) &&
            args.pos.size() != 7U) {
            std::cerr << "Wrong arguments\n";
            return EXIT_FAILURE;
        }

        Environment env(args);
        env.setup();

        result = run(args, env.getTimeKeeper());

        env.teardown();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        result = EXIT_FAILURE;
    }

    if (result != EXIT_SUCCESS && args.gitDiff) {
        if (args.pos[5] == std::string(40U, '0')) {
            execlp("git", "git", "diff", "--no-ext-diff", args.pos[2].c_str(),
                "--", args.pos[0].c_str(), static_cast<char *>(nullptr));
            exit(127);
        }
        execlp("git", "git", "diff", "--no-ext-diff", args.pos[2].c_str(),
            args.pos[5].c_str(), "--", static_cast<char *>(nullptr));
        exit(127);
    }

    return result;
}

static Args
parseLocArgs(const std::vector<std::string> &argv)
{
    po::options_description options;
    options.add_options()
        ("no-refine",   "do not refine coarse results");

    Args args;
    boost::program_options::variables_map varMap = parseArgs(args, argv,
                                                             options);

    args.highlightMode = (args.pos.size() == 1U);
    args.noRefine = varMap.count("no-refine");
    args.gitDiff = (args.pos.size() == 7U);

    return args;
}

static int
run(const Args &args, TimeReport &tr)
{
    cpp17::pmr::monolithic mr;
    Tree treeA(&mr), treeB(&mr);

    const std::string oldFile = (args.gitDiff ? args.pos[1] : args.pos[0]);
    if (optional_t<Tree> &&tree = buildTreeFromFile(oldFile, args, tr, &mr)) {
        treeA = *tree;
    } else {
        return EXIT_FAILURE;
    }

    if (args.highlightMode) {
        dumpTrees(args, treeA, treeB);
        if (!args.dryRun) {
            std::cout << Highlighter().print(*treeA.getRoot()) << '\n';
        }
        return EXIT_SUCCESS;
    }

    const std::string newFile = (args.gitDiff ? args.pos[4] : args.pos[1]);
    if (optional_t<Tree> &&tree = buildTreeFromFile(newFile, args, tr, &mr)) {
        treeB = *tree;
    } else {
        return EXIT_FAILURE;
    }

    if (args.dryRun) {
        dumpTrees(args, treeA, treeB);
        return EXIT_SUCCESS;
    }

    // markSatellites(*treeA.getRoot());
    // markSatellites(*treeB.getRoot());

    Node *T1 = treeA.getRoot(), *T2 = treeB.getRoot();
    compare(T1, T2, tr, !args.fine, args.noRefine);

    dumpTrees(args, treeA, treeB);

    Printer printer(*T1, *T2, std::cout);
    if (args.gitDiff) {
        printer.addHeader({ args.pos[3], args.pos[6] });
        printer.addHeader({ "a/" + args.pos[0], "b/" + args.pos[0] });
    } else {
        printer.addHeader({ oldFile, newFile });
    }
    printer.print(tr);

    // printTree("T1", *T1);
    // printTree("T2", *T2);

    return EXIT_SUCCESS;
}
