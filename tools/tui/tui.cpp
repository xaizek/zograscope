// Copyright (C) 2019 xaizek <xaizek@posteo.net>
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

#include <iostream>
#include <sstream>
#include <vector>

#include "cursed/Init.hpp"
#include "cursed/Input.hpp"
#include "cursed/Label.hpp"
#include "cursed/Placeholder.hpp"
#include "cursed/Screen.hpp"
#include "cursed/Track.hpp"
#include "cursed/utils.hpp"

#include "vle/KeyDispatcher.hpp"
#include "vle/Mode.hpp"
#include "vle/Modes.hpp"

#include "tooling/Traverser.hpp"
#include "tooling/common.hpp"
#include "Highlighter.hpp"
#include "tree.hpp"

#include "FileRegistry.hpp"
#include "ViewManager.hpp"

static int run(const CommonArgs &args, TimeReport &tr);

const char *const usage =
R"(Usage: zs-tui [options...] [paths...]

Paths can specify both files and directories.  When no path is specified, "." is
assumed.

Supported Vim-like shortcuts: G, gg, j and k.

Other shortcuts:
 * files view:
    - c -- enter code view
    - d -- enter dump view
    - f -- switch to functions view
    - q -- quit the application
 * functions view:
    - c -- enter code view
    - d -- enter dump view
    - f -- switch to files view
    - q -- quit the application
 * code view:
    - c/q -- leave code view
 * dump view:
    - d/q -- leave dump view
)";

int
main(int argc, char *argv[])
{
    int result;

    try {
        Environment env;
        env.setup({ argv + 1, argv + argc });

        CommonArgs args = env.getCommonArgs();
        if (args.help) {
            std::cout << usage
                      << "\n"
                      << "Options:\n";
            env.printOptions();
            return EXIT_SUCCESS;
        }
        result = run(args, env.getTimeKeeper());

        env.teardown();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        result = EXIT_FAILURE;
    }

    return result;
}

// Runs the tool.  Returns exit code of the application.
static int
run(const CommonArgs &args, TimeReport &tr)
{
    std::vector<std::string> paths = args.pos;
    if (paths.empty()) {
        paths.emplace_back(".");
    }

    FileRegistry registry(args, tr);

    if (!Traverser(paths, args.lang,
                   [&](const std::string &path) {
                       return registry.addFile(path);
                   }).search()) {
        std::cerr << "No matching files were discovered.\n";
        return EXIT_FAILURE;
    }

    cursed::Init init;
    cursed::Input input(cursed::Keypad::Enabled);

    cursed::Format barBg;
    barBg.setBackground(cursed::Color::White);
    barBg.setForeground(60);
    barBg.setBold(true);
    barBg.setReversed(true);

    cursed::Format helpBg;
    helpBg.setBackground(239);

    cursed::Label title;
    title.setBackground(barBg);
    cursed::Label helpLine;
    helpLine.setBackground(helpBg);
    cursed::Label inputBuf;
    inputBuf.setBackground(barBg);

    cursed::Placeholder viewPlaceholder;

    cursed::Track track;
    track.addItem(&title);
    track.addItem(&viewPlaceholder);
    track.addItem(&helpLine);
    track.addItem(&inputBuf);

    ViewContext viewContext = {
        .registry = registry,
        .quit = false,
        .viewChanged = false,
        .lang = nullptr,
        .node = nullptr,
    };
    ViewManager viewManager(viewContext, viewPlaceholder);
    viewManager.push("files");

    auto viewChanged = [&]() {
        title.setText(L"[" + cursed::toWide(viewManager.getViewName()) + L"]");
        helpLine.setText(viewManager.getViewHelpLine());
        viewContext.viewChanged = false;
    };

    viewChanged();

    cursed::Screen screen;
    screen.replaceTopWidget(&track);
    screen.draw();

    vle::KeyDispatcher dispatcher;
    while (cursed::InputElement ie = input.read()) {
        if (ie.isTerminalResize()) {
            screen.resize();
            screen.draw();
            continue;
        }

        if (!ie.isFunctionalKey()) {
            dispatcher.dispatch(ie);

            if (viewContext.quit) {
                break;
            }

            if (viewContext.viewChanged) {
                viewChanged();
                screen.resize();
            }
        }

        inputBuf.setText(dispatcher.getPendingInput());
        screen.draw();
    }
    return EXIT_SUCCESS;
}
