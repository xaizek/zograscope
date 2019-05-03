// Copyright (C) 2019 xaizek <xaizek@posteo.net>
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

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "cursed/Init.hpp"
#include "cursed/Input.hpp"
#include "cursed/Label.hpp"
#include "cursed/List.hpp"
#include "cursed/ListLike.hpp"
#include "cursed/Screen.hpp"
#include "cursed/Table.hpp"
#include "cursed/Text.hpp"
#include "cursed/Track.hpp"
#include "cursed/utils.hpp"

#include "vle/KeyDispatcher.hpp"
#include "vle/Mode.hpp"
#include "vle/Modes.hpp"

#include "pmr/monolithic.hpp"
#include "tooling/FunctionAnalyzer.hpp"
#include "tooling/Traverser.hpp"
#include "tooling/common.hpp"
#include "utils/optional.hpp"
#include "Highlighter.hpp"
#include "NodeRange.hpp"
#include "decoration.hpp"
#include "mtypes.hpp"
#include "tree.hpp"

namespace {

struct Location
{
    std::string path;
    int line;
    int col;
};

struct FuncInfo
{
    const Node *node;
    Location loc;
    int size;
    int params;
};

class FileRegistry
{
public:
    FileRegistry(const CommonArgs &args, TimeReport &tr);

public:
    bool addFile(const std::string &path);

    const std::vector<FuncInfo> & getFuncInfos() const;
    const Tree & getTree(const std::string &path) const;

private:
    const CommonArgs &args;
    TimeReport &tr;

    cpp17::pmr::monolithic mr;
    std::unordered_map<std::string, Tree> trees;

    std::vector<FuncInfo> infos;
};

}

inline
FileRegistry::FileRegistry(const CommonArgs &args, TimeReport &tr)
    : args(args), tr(tr)
{ }

inline bool
FileRegistry::addFile(const std::string &path)
{
    Tree &tree = trees.emplace(path, Tree(&mr)).first->second;

    if (optional_t<Tree> &&t = buildTreeFromFile(path, args, tr, &mr)) {
        tree = *t;
    } else {
        std::cerr << "Failed to parse: " << path << '\n';
        return false;
    }

    dumpTree(args, tree);

    if (args.dryRun) {
        return true;
    }

    Language &lang = *tree.getLanguage();

    FunctionAnalyzer functionAnalyzer(lang);
    Location loc = { path, -1, -1 };
    for (const Node *node : NodeRange(tree.getRoot())) {
        if (!node->leaf && lang.classify(node->stype) == MType::Function) {
            loc.line = node->line;
            loc.col = node->col;

            int size = functionAnalyzer.getLineCount(node);
            int params = functionAnalyzer.getParamCount(node);
            infos.emplace_back(FuncInfo { node, loc, size, params });
        }
    }

    return true;
}

inline const std::vector<FuncInfo> &
FileRegistry::getFuncInfos() const
{
    return infos;
}

inline const Tree &
FileRegistry::getTree(const std::string &path) const
{
    return trees.at(path);
}

static int run(const CommonArgs &args, TimeReport &tr);

const char *const usage =
R"(Usage: zs-tui [options...] [paths...]

Paths can specify both files and directories.  When no path is specified, "." is
assumed.

Supported Vim-like shortcuts: G, gg, j and k.

Other shortcuts:
 - c -- enter/leave code view
 - d -- enter/leave dump view
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

// Populates mode with typical list operations.
static void
addListOperations(vle::Mode &mode, cursed::ListLike &list)
{
    mode.addShortcut({ L"G", [&](int count) {
        if (count < 0) {
            list.moveToLast();
        } else {
            list.moveToPos(count - 1);
        }
    }, "go to the last or [count]-th item" });
    mode.addShortcut({ L"gg", [&](int count) {
        if (count < 0) {
            list.moveToFirst();
        } else {
            list.moveToPos(count - 1);
        }
    }, "go to the first or [count]-th item" });
    mode.addShortcut({ L"j", [&](int count) {
        list.moveDown(count < 0 ? 1 : count);
    }, "go [count] items below" });
    mode.addShortcut({ L"k", [&](int count) {
        list.moveUp(count < 0 ? 1 : count);
    }, "go [count] items above" });
}

// Populates mode with typical text operations.
static void
addTextOperations(vle::Mode &mode, cursed::Text &text)
{
    mode.addShortcut({ L"G", [&]() {
        text.scrollToBottom();
    }, "scroll to the top" });
    mode.addShortcut({ L"gg", [&]() {
        text.scrollToTop();
    }, "scroll to the bottom" });
    mode.addShortcut({ L"j", [&]() {
        text.scrollDown();
    }, "scroll one line down" });
    mode.addShortcut({ L"k", [&]() {
        text.scrollUp();
    }, "scroll one line up" });
}

// Runs the tool.  Returns exit code of the application.
static int
run(const CommonArgs &args, TimeReport &tr)
{
    decor::disableDecorations();

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

    cursed::Format titleBg;
    titleBg.setBackground(cursed::Color::Black);
    titleBg.setForeground(cursed::Color::Cyan);
    titleBg.setBold(true);
    titleBg.setReversed(true);

    cursed::Label title;
    title.setText(L"Functions");
    title.setBackground(titleBg);
    cursed::Label otherTitle;
    otherTitle.setBackground(titleBg);
    cursed::Label inputBuf;
    inputBuf.setBackground(titleBg);

    cursed::Format path;
    path.setBold(true);
    cursed::Format lineNo;
    cursed::Format colNo;

    cursed::Format header;
    header.setBold(true);
    header.setReversed(true);

    cursed::Table table({ { header(L" LOCATION "), cursed::Align::Left },
                          { header(L" SIZE "), cursed::Align::Right },
                          { header(L" PARAMS "), cursed::Align::Right } });

    const std::vector<FuncInfo> &infos = registry.getFuncInfos();
    for (const FuncInfo &info : infos) {
        const Location &loc = info.loc;
        table.append({ path(cursed::toWide(loc.path)) +
                       L":" + lineNo(std::to_wstring(loc.line)) +
                       L":" + lineNo(std::to_wstring(loc.col)),
                       std::to_wstring(info.size),
                       std::to_wstring(info.params) });
    }

    cursed::Text info;

    cursed::Track mainTrack;
    mainTrack.addItem(&title);
    mainTrack.addItem(&table);
    mainTrack.addItem(&inputBuf);

    cursed::Track dumpTrack;
    dumpTrack.addItem(&otherTitle);
    dumpTrack.addItem(&info);
    dumpTrack.addItem(&inputBuf);

    cursed::Track codeTrack;
    codeTrack.addItem(&otherTitle);
    codeTrack.addItem(&info);
    codeTrack.addItem(&inputBuf);

    cursed::Screen screen;
    screen.replaceTopWidget(&mainTrack);
    screen.draw();

    bool quit = false;
    vle::Modes modes;

    vle::Mode normalMode("normal");
    normalMode.setUsesCount(true);
    addListOperations(normalMode, table);
    normalMode.addShortcut({ L"q", [&]() {
        quit = true;
    }, "quit the application" });
    normalMode.addShortcut({ L"d", [&]() {
        modes.switchTo("dump");
        otherTitle.setText(L"Dump");
        screen.replaceTopWidget(&dumpTrack);

        std::stringstream oss;
        const FuncInfo &fi = infos[table.getPos()];
        dumpTree(oss, fi.node, registry.getTree(fi.loc.path).getLanguage());

        std::vector<cursed::ColorTree> lines;
        for (std::string line; std::getline(oss, line); ) {
            lines.push_back(cursed::toWide(line));
        }
        info.setLines(std::move(lines));
    }, "show dump dialog" });

    normalMode.addShortcut({ L"c", [&]() {
        modes.switchTo("code");
        otherTitle.setText(L"Code");
        screen.replaceTopWidget(&codeTrack);

        const FuncInfo &fi = infos[table.getPos()];
        Highlighter hi(*fi.node,
                       *registry.getTree(fi.loc.path).getLanguage(),
                       true, fi.node->line);

        std::vector<cursed::ColorTree> lines;
        for (const ColorCane &cc : hi.print().splitIntoLines()) {
            std::string line;
            for (const ColorCanePiece &piece : cc) {
                line += piece.text;
            }
            lines.push_back(cursed::toWide(line));
        }
        info.setLines(std::move(lines));
    }, "show code dialog" });

    vle::Mode dumpMode("dump");
    addTextOperations(dumpMode, info);
    dumpMode.addShortcut({ L"d", [&]() {
        modes.switchTo("normal");
        screen.replaceTopWidget(&mainTrack);
    }, "return to previous view" });

    vle::Mode codeMode("code");
    addTextOperations(codeMode, info);
    codeMode.addShortcut({ L"c", [&]() {
        modes.switchTo("normal");
        screen.replaceTopWidget(&mainTrack);
    }, "return to previous view" });

    std::vector<vle::Mode> allModes;
    allModes.emplace_back(std::move(normalMode));
    allModes.emplace_back(std::move(dumpMode));
    allModes.emplace_back(std::move(codeMode));
    modes.setModes(std::move(allModes));
    modes.switchTo("normal");

    cursed::Input input(cursed::Keypad::Enabled);

    vle::KeyDispatcher dispatcher;

    while (cursed::InputElement ie = input.read()) {
        if (ie.isTerminalResize()) {
            screen.resize();
            screen.draw();
            continue;
        }

        if (!ie.isFunctionalKey()) {
            dispatcher.dispatch(ie);

            if (quit) {
                break;
            }
        }

        inputBuf.setText(dispatcher.getPendingInput());
        screen.draw();
    }
    return EXIT_SUCCESS;
}
