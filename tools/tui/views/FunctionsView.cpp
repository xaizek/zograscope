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

#include "FunctionsView.hpp"

#include "cursed/Table.hpp"
#include "cursed/utils.hpp"

#include "vle/Mode.hpp"

#include "../FileRegistry.hpp"
#include "../ViewManager.hpp"
#include "../common.hpp"

FunctionsView::FunctionsView(ViewManager &manager) : View(manager, "functions")
{
    cursed::Format header;
    header.setBold(true);
    header.setReversed(true);

    table.addColumn({ header(L" LOCATION "), cursed::Align::Left });
    table.addColumn({ header(L" SIZE "), cursed::Align::Right });
    table.addColumn({ header(L" PARAMS "), cursed::Align::Right });

    track.addItem(&table);

    helpLine = buildShortcut(L"f", L"list files")
             + buildShortcut(L"c", L"show as code")
             + buildShortcut(L"d", L"show as dump")
             + buildShortcut(L"q", L"quit");
}

vle::Mode
FunctionsView::buildMode()
{
    vle::Mode mode(getName());
    mode.setUsesCount(true);

    addListOperations(mode, table);

    mode.addShortcut({ L"q", [&]() {
        context.quit = true;
    }, "quit the application" });

    mode.addShortcut({ L"f", [&]() {
        manager.push("files");
    }, "show files view" });

    mode.addShortcut({ L"d", [&]() {
        goToInfoMode("dump");
    }, "show dump view" });

    mode.addShortcut({ L"c", [&]() {
        goToInfoMode("code");
    }, "show code view" });

    return mode;
}

void
FunctionsView::goToInfoMode(const std::string &mode)
{
    const std::vector<FuncInfo> &infos = context.registry.getFuncInfos();
    const FuncInfo &fi = infos[table.getPos()];
    context.node = fi.node;
    context.lang = context.registry.getTree(fi.loc.path).getLanguage();
    manager.push(mode);
}

void
FunctionsView::update()
{
    cursed::Format path;
    path.setBold(true);
    cursed::Format lineNo;
    cursed::Format colNo;

    const std::vector<FuncInfo> &infos = context.registry.getFuncInfos();
    for (const FuncInfo &info : infos) {
        const Location &loc = info.loc;
        table.append({ path(cursed::toWide(loc.path)) +
                       L":" + lineNo(std::to_wstring(loc.line)) +
                       L":" + colNo(std::to_wstring(loc.col)),
                       std::to_wstring(info.size),
                       std::to_wstring(info.params) });
    }
}
