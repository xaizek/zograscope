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

#include "tooling/Finder.hpp"
#include "tooling/common.hpp"
#include "Args.hpp"

static boost::program_options::options_description getLocalOpts();
static Args parseLocalArgs(const Environment &env);
static int run(const Args &args, TimeReport &tr);

const char *const usage =
R"(Usage: zs-find [options...] [paths...] : matchers...
   or: zs-find [options...] [paths...] : [matchers...] : tokens...
   or: zs-find [options...] [paths...] :: tokens...

Paths can specify both files and directories.  When no path is specified, "." is
assumed.

Each of tokens is just a value of a token like `[` or `int`.

Available matchers:
   decl   Any sort of declaration
   stmt   Statement
   func   Functions (their definitions only)
   param  Parameter of a function
   comm   Comments of any kind
   dir    Preprocessor-alike directives
)";

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
            std::cout << usage
                      << "\n"
                      << "Options:\n";
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
        foundSomething = Finder(args, tr, args.count).search();
    }
    return (foundSomething ? EXIT_SUCCESS : EXIT_FAILURE);
}
