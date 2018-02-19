// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include <iostream>

#include "Args.hpp"
#include "Finder.hpp"
#include "common.hpp"

// TODO: allow processing of multiple files specified on the command line?

static boost::program_options::options_description getLocalOpts();
static Args parseLocalArgs(const Environment &env);
static int run(const Args &args, TimeReport &tr);

int
main(int argc, char *argv[])
{
    Args args;
    int result;

    try {
        Environment env(getLocalOpts());
        env.setup({ argv + 1, argv + argc });

        Args args = parseLocalArgs(env);
        if (args.help) {
            env.printOptions();
            return EXIT_SUCCESS;
        }
        if (args.pos.size() < 2U) {
            std::cerr << "Wrong arguments\n";
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

// Retrieves description of options specific to this tool.
static boost::program_options::options_description
getLocalOpts()
{
    boost::program_options::options_description options;
    options.add_options()
        ("count,c", "only count matches and report statistics");

    return options;
}

// Parses options specific to the tool.
static Args
parseLocalArgs(const Environment &env)
{
    Args args;
    static_cast<CommonArgs &>(args) = env.getCommonArgs();

    const boost::program_options::variables_map &varMap = env.getVarMap();

    args.count = varMap.count("count");

    return args;
}

// Runs the tool.  Returns exit code of the application.
static int
run(const Args &args, TimeReport &tr)
{
    int foundSomething = false;
    if (!args.dryRun) {
        Finder finder(args, tr);
        foundSomething = finder.find(args.pos[0]);
    }
    return (foundSomething ? EXIT_SUCCESS : EXIT_FAILURE);
}
