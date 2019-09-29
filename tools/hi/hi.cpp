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

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "pmr/monolithic.hpp"

#include "tooling/common.hpp"
#include "utils/optional.hpp"
#include "TermHighlighter.hpp"
#include "tree.hpp"

static int run(const CommonArgs &args, TimeReport &tr);

int
main(int argc, char *argv[])
{
    int result;

    try {
        Environment env;
        env.setup({ argv + 1, argv + argc });

        CommonArgs args = env.getCommonArgs();
        if (args.help) {
            std::cout << "Usage: zs-hi [options...] [file|-]\n"
                      << "\n"
                      << "Options:\n";
            env.printOptions();
            return EXIT_SUCCESS;
        }
        if (args.pos.size() > 1U) {
            env.teardown(true);
            std::cerr << "Wrong positional arguments\n"
                      << "Expected at most one\n";
            return EXIT_FAILURE;
        }

        result = run(args, env.getTimeKeeper());

        env.teardown();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        result = EXIT_FAILURE;
    }

    return result;
}

static int
run(const CommonArgs &args, TimeReport &tr)
{
    cpp17::pmr::monolithic mr;
    Tree tree(&mr);

    if (args.pos.empty() || args.pos[0] == "-") {
        std::ostringstream oss;
        oss << std::cin.rdbuf();
        std::string contents = oss.str();

        if (optional_t<Tree> &&t = buildTreeFromFile("<input>", contents, args,
                                                     tr, &mr)) {
            tree = *t;
        } else {
            std::cerr << "Failed to parse standard input\n";
            return EXIT_FAILURE;
        }
    } else {
        const std::string &path = args.pos[0];
        if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
            tree = *t;
        } else {
            std::cerr << "Failed to parse: " << path << '\n';
            return EXIT_FAILURE;
        }
    }

    dumpTree(args, tree);
    if (!args.dryRun) {
        std::cout << TermHighlighter(tree).print() << '\n';
    }
    return EXIT_SUCCESS;
}
