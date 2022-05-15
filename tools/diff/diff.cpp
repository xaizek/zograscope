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

#include <boost/optional.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <algorithm>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pmr/monolithic.hpp"

#include "tooling/common.hpp"
#include "utils/optional.hpp"
#include "Printer.hpp"
#include "compare.hpp"
#include "decoration.hpp"
#include "tree.hpp"

// Tool-specific type for holding arguments.
struct Args : CommonArgs
{
    bool noRefine;      // Don't run TED on updated nodes.
    bool gitDiff;       // Invoked by git and file was changed.
    bool gitRename;     // File was renamed and possibly changed too.
    bool gitRenameOnly; // File was renamed without changing it.
};

static boost::program_options::options_description getLocalOpts();
static Args parseLocalArgs(const Environment &env);
static int run(Environment &env, const Args &args);
static int gitFallback(const Args &args);

int
main(int argc, char *argv[])
{
    Args args = { };
    int result;

    try {
        Environment env(getLocalOpts());
        env.setup({ argv + 1, argv + argc });

        args = parseLocalArgs(env);
        if (args.help) {
            std::cout << "Usage: zs-diff [options...] old-file new-file\n"
                      << "   or: zs-diff [options...] <7 or 9 args from git>\n"
                      << "\n"
                      << "Options:\n";
            env.printOptions();
            return EXIT_SUCCESS;
        }
        if (args.pos.size() != 2U && !args.gitDiff && !args.gitRename) {
            env.teardown(true);
            std::cerr << "Wrong positional arguments\n"
                      << "Expected 2 (cli) or 7 or 9 (git)\n";
            return EXIT_FAILURE;
        }

        result = run(env, args);

        env.teardown();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        result = EXIT_FAILURE;
    }

    if (result != EXIT_SUCCESS && args.gitDiff) {
        return gitFallback(args);
    }

    return result;
}

static boost::program_options::options_description
getLocalOpts()
{
    boost::program_options::options_description options;
    options.add_options()
        ("no-refine", "do not refine coarse results");

    return options;
}

static Args
parseLocalArgs(const Environment &env)
{
    Args args;
    static_cast<CommonArgs &>(args) = env.getCommonArgs();

    const boost::program_options::variables_map &varMap = env.getVarMap();

    args.noRefine = varMap.count("no-refine");
    args.gitDiff = args.pos.size() == 7U
                || (args.pos.size() == 9U && args.pos[2] != args.pos[5]);
    args.gitRename = (args.pos.size() == 9U);
    args.gitRenameOnly = (args.gitRename && args.pos[2] == args.pos[5]);

    return args;
}

static int
run(Environment &env, const Args &args)
{
    if (args.gitRenameOnly) {
        std::cout << (decor::bold << "{ renamed without changes }\n")
                  << (decor::bold << "  old name: " << args.pos[0]) << '\n'
                  << (decor::bold << "  new name: " << args.pos[7]) << '\n';
        return EXIT_SUCCESS;
    }

    cpp17::pmr::monolithic mrA, mrB;
    Tree treeA(&mrA), treeB(&mrB);

    using overload = optional_t<Tree> (*)(Environment &,
                                          TimeReport &,
                                          const Attrs &,
                                          const std::string &,
                                          cpp17::pmr::memory_resource *);
    overload func = &buildTreeFromFile;

    const std::string oldFile = (args.gitDiff ? args.pos[1] : args.pos[0]);
    const std::string newFile = (args.gitDiff ? args.pos[4] : args.pos[1]);

    // New file should be in-tree.
    Attrs attrs = env.getConfig().lookupAttrs(newFile);

    TimeReport &tr = env.getTimeKeeper();
    TimeReport nestedTr(tr);
    std::future<optional_t<Tree>> newTreeFuture = std::async(std::launch::async,
                                                             func,
                                                             std::ref(env),
                                                             std::ref(nestedTr),
                                                             attrs,
                                                             newFile,
                                                             &mrB);

    if (optional_t<Tree> &&tree = func(env, tr, attrs, oldFile, &mrA)) {
        treeA = *tree;
    } else {
        // Wait the other thread to finish to avoid data races.
        newTreeFuture.wait();

        std::cerr << "Failed to parse: " << oldFile << '\n';
        return EXIT_FAILURE;
    }

    if (optional_t<Tree> &&tree = newTreeFuture.get()) {
        treeB = *tree;
    } else {
        std::cerr << "Failed to parse: " << newFile << '\n';
        return EXIT_FAILURE;
    }

    if (args.dryRun) {
        dumpTrees(args, treeA, treeB);
        return EXIT_SUCCESS;
    }

    compare(treeA, treeB, tr, !args.fine, args.noRefine);

    dumpTrees(args, treeA, treeB);

    Printer printer(*treeA.getRoot(), *treeB.getRoot(), *treeA.getLanguage(),
                    std::cout);
    if (args.gitDiff) {
        printer.addHeader({ args.pos[3], args.pos[6] });
        const int newNameIdx = (args.gitRename ? 7 : 0);
        printer.addHeader({ "a/" + args.pos[0], "b/" + args.pos[newNameIdx] });
    } else {
        printer.addHeader({ oldFile, newFile });
    }
    printer.print(tr);

    return EXIT_SUCCESS;
}

static int
gitFallback(const Args &args)
{
    if (args.pos[5] == std::string(40U, '0')) {
        execlp("git", "git", "diff", "--no-ext-diff", args.pos[2].c_str(),
               "--", args.pos[0].c_str(), static_cast<char *>(nullptr));
        return 127;
    }
    execlp("git", "git", "diff", "--no-ext-diff", args.pos[2].c_str(),
           args.pos[5].c_str(), "--", static_cast<char *>(nullptr));
    return 127;
}
