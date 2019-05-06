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

#include "DumpView.hpp"

#include <sstream>
#include <vector>

#include "cursed/ListLike.hpp"
#include "cursed/Text.hpp"
#include "cursed/utils.hpp"

#include "vle/Mode.hpp"

#include "tree.hpp"

#include "../ViewManager.hpp"
#include "../common.hpp"

DumpView::DumpView(ViewManager &manager) : View(manager, "dump")
{
    track.addItem(&text);
}

vle::Mode
DumpView::buildMode()
{
    vle::Mode mode(getName());
    addTextOperations(mode, text);

    mode.addShortcut({ std::vector<std::wstring> { L"d", L"q" }, [&]() {
        manager.pop();
    }, "return to previous view" });

    return mode;
}

void
DumpView::update()
{
    std::stringstream oss;
    dumpTree(oss, context.node, context.lang);

    std::vector<cursed::ColorTree> lines;
    for (std::string line; std::getline(oss, line); ) {
        lines.push_back(cursed::toWide(line));
    }
    text.setLines(std::move(lines));
}
