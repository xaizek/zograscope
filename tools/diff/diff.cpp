// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pmr/monolithic.hpp"

#include "utils/optional.hpp"
#include "Highlighter.hpp"
#include "Printer.hpp"
#include "common.hpp"
#include "compare.hpp"
#include "decoration.hpp"
#include "tree.hpp"

struct Args : CommonArgs
{
    bool noRefine;
    bool gitDiff;
    bool gitRename;
};

static boost::program_options::options_description getLocalOpts();
static Args parseLocalArgs(const Environment &env);
static int run(const Args &args, TimeReport &tr);

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
            env.printOptions();
            return EXIT_SUCCESS;
        }
        if (args.pos.size() != 2U && !args.gitDiff && !args.gitRename) {
            env.teardown(true);
            std::cerr << "Wrong positional arguments\n"
                      << "Expected 2 (cli) or 7 or 9 (git)\n";
            return EXIT_FAILURE;
        }

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
    args.gitRename = (args.pos.size() == 9U && args.pos[2] == args.pos[5]);

    return args;
}

static int
run(const Args &args, TimeReport &tr)
{
    if (args.gitRename) {
        std::cout << (decor::bold << "{ old name } " << args.pos[0]) << '\n'
                  << (decor::bold << "{ new name } " << args.pos[7]) << '\n';
        return EXIT_SUCCESS;
    }

    cpp17::pmr::monolithic mr;
    Tree treeA(&mr), treeB(&mr);

    const std::string oldFile = (args.gitDiff ? args.pos[1] : args.pos[0]);
    if (optional_t<Tree> &&tree = buildTreeFromFile(oldFile, args, tr, &mr)) {
        treeA = *tree;
    } else {
        return EXIT_FAILURE;
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

    compare(treeA, treeB, tr, !args.fine, args.noRefine);

    dumpTrees(args, treeA, treeB);

    Printer printer(*treeA.getRoot(), *treeB.getRoot(), std::cout);
    if (args.gitDiff) {
        printer.addHeader({ args.pos[3], args.pos[6] });
        printer.addHeader({ "a/" + args.pos[0], "b/" + args.pos[0] });
    } else {
        printer.addHeader({ oldFile, newFile });
    }
    printer.print(tr);

    return EXIT_SUCCESS;
}
