#include "common.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/optional.hpp>
#include "pmr/monolithic.hpp"

#include "c/parser.hpp"
#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "CommonArgs.hpp"
#include "TreeBuilder.hpp"
#include "STree.hpp"
#include "decoration.hpp"
#include "tree.hpp"

static std::string readFile(const std::string &path);

void
Environment::setup()
{
    if (args->color) {
        decor::enableDecorations();
    }
}

void
Environment::teardown()
{
    if (args->timeReport) {
        tr.stop();
        std::cout << tr;
    }
}

optional_t<Tree>
buildTreeFromFile(const std::string &path, const CommonArgs &args,
                  TimeReport &tr, cpp17::pmr::memory_resource *mr)
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
        t = Tree(contents, tb.getRoot(), &localMR);
    } else {
        STree stree(std::move(tb), contents, args.dumpSTree, args.sdebug,
                    localMR);
        t = Tree(contents, stree.getRoot(), mr);
    }

    return optional_t<Tree>(std::move(t));
}

static std::string
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

void
dumpTree(const CommonArgs &args, Tree &tree)
{
    if (args.dumpTree) {
        if (Node *root = tree.getRoot()) {
            std::cout << "Tree:\n";
            print(*root);
        }
    }
}

void
dumpTrees(const CommonArgs &args, Tree &treeA, Tree &treeB)
{
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
}
