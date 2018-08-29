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

#include <iostream>

#include "tooling/common.hpp"

#include "ZSDiff.hpp"

int
main(int argc, char *argv[]) try
{
    QApplication app(argc, argv);

    Environment env;
    env.setup({ argv + 1, argv + argc });

    const CommonArgs &args = env.getCommonArgs();

    if (args.help) {
        std::cout << "Usage: zs-gdiff [options...] old-file new-file\n"
                  << "   or: zs-gdiff [options...] <7 or 9 args from git>\n"
                  << "\n"
                  << "Options:\n";
        env.printOptions();
        return EXIT_SUCCESS;
    }
    if (args.pos.size() != 2U && args.pos.size() != 7U &&
        args.pos.size() != 9U) {
        env.teardown(true);
        std::cerr << "Wrong positional arguments\n"
                  << "Expected 2 (cli) or 7 or 9 (git)\n";
        return EXIT_FAILURE;
    }

    const bool git = (args.pos.size() != 2U);
    const std::string &oldFile = (git ? args.pos[1] : args.pos[0]);
    const std::string &newFile = (git ? args.pos[4] : args.pos[1]);

    ZSDiff w(oldFile, newFile, env.getTimeKeeper());
    w.show();

    int result = app.exec();
    env.teardown();
    return result;
} catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
}
