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

#include "FilesView.hpp"

#include <algorithm>
#include <utility>

#include "cursed/List.hpp"
#include "cursed/utils.hpp"

#include "../FileRegistry.hpp"
#include "../ViewManager.hpp"
#include "../common.hpp"

FilesView::FilesView(ViewManager &manager) : View(manager, "files")
{
    track.addItem(&list);

    helpLine = buildShortcut(L"f", L"list functions")
             + buildShortcut(L"c", L"show as code")
             + buildShortcut(L"d", L"show as dump")
             + buildShortcut(L"q", L"quit");
}

vle::Mode
FilesView::buildMode()
{
    vle::Mode mode(getName());
    mode.setUsesCount(true);

    addListOperations(mode, list);

    mode.addShortcut({ L"q", [&]() {
        context.quit = true;
    }, "quit the application" });

    mode.addShortcut({ L"f", [&]() {
        manager.push("functions");
    }, "show functions view" });

    mode.addShortcut({ L"d", [&]() {
        goToInfoMode("dump");
    }, "show dump view" });

    mode.addShortcut({ L"c", [&]() {
        goToInfoMode("code");
    }, "show code view" });

    return mode;
}

void
FilesView::goToInfoMode(const std::string &mode)
{
    const std::string &fileName = files[list.getPos()];
    const Tree &tree = context.registry.getTree(fileName);
    context.node = tree.getRoot();
    context.lang = tree.getLanguage();
    manager.push(mode);
}

void
FilesView::update()
{
    files = context.registry.listFileNames();
    std::sort(files.begin(), files.end());

    std::vector<cursed::ColorTree> lines;
    lines.reserve(files.size());

    cursed::Format path;
    path.setBold(true);

    for (const std::string &fileName : files) {
        lines.push_back(path(cursed::toWide(fileName)));
    }

    list.setItems(std::move(lines));
}
