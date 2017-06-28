#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Printer.hpp"
#include "TreeBuilder.hpp"
#include "change-distilling.hpp"
#include "decoration.hpp"
#include "integration.hpp"
#include "parser.hpp"
#include "tree.hpp"
#include "tree-edit-distance.hpp"
#include "types.hpp"

namespace po = boost::program_options;

static po::variables_map parseOptions(const std::vector<std::string> &args);

class TimeReport
{
    using clock = std::chrono::high_resolution_clock;

    struct Measure
    {
        Measure(std::string &&stage, clock::time_point start)
            : stage(std::move(stage)), start(start)
        {
        }

        std::string stage;
        clock::time_point start;
        clock::time_point end;
    };

    class ProxyTimer
    {
    public:
        ProxyTimer(TimeReport &tr) : tr(tr) { }
        ~ProxyTimer() try
        {
            tr.stop();
        } catch (...) {
            // Do not throw from a destructor.
        }

    private:
        TimeReport &tr;
    };

    friend inline std::ostream &
    operator<<(std::ostream &os, const TimeReport &tr)
    {
        for (const Measure &measure : tr.measures) {
            using seconds = std::chrono::duration<float>;
            auto duration =
                std::chrono::nanoseconds(measure.end - measure.start);
            os << measure.stage << " -- " << seconds(duration).count() << "s\n";
        }
        return os;
    }

public:
    ProxyTimer measure(const std::string &stage)
    {
        start(stage);
        return ProxyTimer(*this);
    }

    void start(std::string stage)
    {
        currentMeasure.emplace(std::move(stage), clock::now());
    }

    void stop()
    {
        if (!currentMeasure) {
            throw std::logic_error("Can't stop timer that isn't running");
        }

        currentMeasure->end = clock::now();
        measures.push_back(*currentMeasure);
        currentMeasure.reset();
    }

private:
    boost::optional<Measure> currentMeasure;
    std::vector<Measure> measures;
};

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
    auto nonTerminal = [](const Node &node) {
        return node.line == 0 || node.col == 0 || !node.children.empty();
    };
    auto terminal = [&nonTerminal](const Node &node) {
        return !nonTerminal(node);
    };

    if (std::any_of(node.children.cbegin(), node.children.cend(), nonTerminal)
        || std::all_of(node.children.cbegin(), node.children.cend(), terminal)) {
        for (Node &child : node.children) {
            child.satellite = !nonTerminal(child);
            markSatellites(child);
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
    const bool dumpTree = varMap.count("dump-tree");
    const bool skipDiff = varMap.count("skip-diff");
    const bool color = varMap.count("color");
    const bool coarse = varMap.count("coarse");
    const bool timeReport = varMap.count("time-report");
    const std::string oldFile = (args.size() == 7U ? args[1] : args[0]);

    if (color) {
        decor::enableDecorations();
    }

    RedirectToPager redirectToPager;
    TimeReport tr;

    Node treeA, treeB;

    std::cout << ">>> Parsing " << oldFile << "\n";
    {
        std::string contents = readFile(oldFile);
        TreeBuilder tb = parse(contents);
        if (tb.hasFailed()) {
            return EXIT_FAILURE;
        }

        treeA = coarse ? materializeTree(contents, tb.makeSTree())
                       : materializeTree(contents, tb.getRoot());
    }

    if (highlightMode) {
        if (dumpTree) {
            print(treeA);
        }

        std::cout << printSource(treeA) << '\n';
        return EXIT_SUCCESS;
    }

    const std::string newFile = (args.size() == 7U ? args[4] : args[1]);
    std::cout << ">>> Parsing " << newFile << "\n";
    {
        std::string contents = readFile(newFile);
        TreeBuilder tb = parse(contents);
        if (tb.hasFailed()) {
            return EXIT_FAILURE;
        }

        treeB = coarse ? materializeTree(contents, tb.makeSTree())
                       : materializeTree(contents, tb.getRoot());
    }

    auto dumpTrees = [&]() {
        if (!dumpTree) {
            return;
        }

        std::cout << "T1\n";
        print(treeA);
        std::cout << "T2\n";
        print(treeB);
    };

    if (skipDiff) {
        dumpTrees();

        std::cout << ">>> Skipping diffing\n";
        return EXIT_SUCCESS;
    }

    // markSatellites(treeA);
    // markSatellites(treeB);

    Node *T1 = &treeA, *T2 = &treeB;
    tr.measure("reduction"), reduceTrees(T1, T2);

    if (coarse) {
        auto timer = tr.measure("diffing");
        distill(*T1, *T2);
    } else {
        auto timer = tr.measure("diffing");
        std::cout << "TED(T1, T2) = " << ted(*T1, *T2) << '\n';
    }

    dumpTrees();

    Printer printer(treeA, treeB);
    tr.measure("printing"), printer.print();

    // printTree("T1", treeA);
    // printTree("T2", treeB);

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
        ("skip-diff", "just parse")
        ("dump-tree", "display tree(s)")
        ("coarse", "use coarse-grained tree")
        ("time-report",  "report time spent on different activities")
        ("color", "force colorization of output");

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
