#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pmr/monolithic.hpp"

#include "utils/optional.hpp"
#include "CommonArgs.hpp"
#include "Highlighter.hpp"
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

struct Args : CommonArgs
{
    bool highlightMode;
    bool noRefine;
    bool gitDiff;
};

static Args parseLocArgs(const std::vector<std::string> &argv);
static int run(const Args &args, TimeReport &tr);
static optional_t<Tree> buildTreeFromFile(const std::string &path,
                                          const Args &args, TimeReport &tr,
                                          cpp17::pmr::memory_resource *mr);

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

        RedirectToPager redirectToPager;
        TimeReport tr;

        result = run(args, tr);

        if (args.timeReport) {
            tr.stop();
            std::cout << tr;
        }
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

static int
run(const Args &args, TimeReport &tr)
{
    if (args.color) {
        decor::enableDecorations();
    }

    cpp17::pmr::monolithic mr;
    Tree treeA(&mr), treeB(&mr);
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
    if (optional_t<Tree> &&tree = buildTreeFromFile(oldFile, args, tr, &mr)) {
        treeA = *tree;
    } else {
        return EXIT_FAILURE;
    }

    if (args.highlightMode) {
        dumpTrees();
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
        dumpTrees();
        return EXIT_SUCCESS;
    }

    // markSatellites(*treeA.getRoot());
    // markSatellites(*treeB.getRoot());

    Node *T1 = treeA.getRoot(), *T2 = treeB.getRoot();
    compare(T1, T2, tr, !args.fine, args.noRefine);

    dumpTrees();

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

/**
 * @brief Reads and parses a file to build its tree.
 *
 * @param path  Path to the file to read.
 * @param args  Arguments of the application.
 * @param tr    Time keeper.
 * @param mr    Allocator.
 *
 * @returns Tree on success or empty optional on error.
 */
static optional_t<Tree>
buildTreeFromFile(const std::string &path, const Args &args, TimeReport &tr,
                  cpp17::pmr::memory_resource *mr)
{
    auto timer = tr.measure("parsing: " + path);

    const std::string contents = readFile(path);

    cpp17::pmr::monolithic localMR;

    TreeBuilder tb = parse(contents, path, args.debug, localMR);
    if (tb.hasFailed()) {
        return {};
    }

    Tree t(mr);

    if (args.fine) {
        t = Tree(contents, tb.getRoot(), mr);
    } else {
        STree stree(std::move(tb), contents, args.dumpSTree, args.sdebug,
                    localMR);
        t = Tree(contents, stree.getRoot(), mr);
    }

    return optional_t<Tree>(std::move(t));
}
