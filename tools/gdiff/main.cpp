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

#include <QApplication>

#include <cstdlib>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>
#include <utility>

#include "tooling/common.hpp"

#include "DiffList.hpp"
#include "Repository.hpp"
#include "ZSDiff.hpp"

// Tool-specific type for holding arguments.
struct Args : CommonArgs
{
    bool staged; // Use staged changes instead of unstaged.
};

static boost::program_options::options_description getLocalOpts();
static Args parseLocalArgs(const Environment &env);

int
main(int argc, char *argv[]) try
{
    QApplication app(argc, argv);

    Environment env(getLocalOpts());
    env.setup({ argv + 1, argv + argc });

    Args args = parseLocalArgs(env);

    if (args.help) {
        std::cout << "Usage: zs-gdiff [options...] [--cached]\n"
                  << "   or: zs-gdiff [options...] old-file new-file\n"
                  << "   or: zs-gdiff [options...] <7 or 9 args from git>\n"
                  << "\n"
                  << "Options:\n";
        env.printOptions();
        return EXIT_SUCCESS;
    }
    if (args.pos.size() != 0U && args.pos.size() != 2U &&
        args.pos.size() != 7U && args.pos.size() != 9U) {
        env.teardown(true);
        std::cerr << "Wrong positional arguments\n"
                  << "Expected 2 (cli) or 7 or 9 (git)\n";
        return EXIT_FAILURE;
    }

    DiffList diffList;
    LaunchMode launchMode;
    if (args.pos.size() == 0U) {
        launchMode = (args.staged ? LaunchMode::Staged : LaunchMode::Unstaged);
        for (DiffEntry &diffEntry : Repository(".").listStatus(args.staged)) {
            diffList.add(std::move(diffEntry));
        }
    } else {
        launchMode = args.pos.size() == 2U ? LaunchMode::Standalone
                                           : LaunchMode::GitExt;
        DiffEntry diffEntry = {
            (launchMode == LaunchMode::GitExt ? args.pos[1] : args.pos[0]),
            (launchMode == LaunchMode::GitExt ? args.pos[4] : args.pos[1])
        };
        diffList.add(std::move(diffEntry));
    }

    if (diffList.empty()) {
        std::cout << "No changed files were discovered\n";
        return EXIT_SUCCESS;
    }

    ZSDiff w(launchMode, std::move(diffList), env.getTimeKeeper());
    w.show();

    int result = app.exec();
    env.teardown();
    return result;
} catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
}

static boost::program_options::options_description
getLocalOpts()
{
    boost::program_options::options_description options;
    options.add_options()
        ("cached", "use staged changes instead of unstaged");

    return options;
}

static Args
parseLocalArgs(const Environment &env)
{
    Args args;
    static_cast<CommonArgs &>(args) = env.getCommonArgs();

    const boost::program_options::variables_map &varMap = env.getVarMap();

    args.staged = varMap.count("cached");

    return args;
}
