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

#include "CodeView.hpp"

#include <sstream>
#include <vector>

#include "cursed/ListLike.hpp"
#include "cursed/Text.hpp"
#include "cursed/utils.hpp"

#include "vle/Mode.hpp"

#include "utils/strings.hpp"
#include "TermHighlighter.hpp"
#include "tree.hpp"

#include "../ViewManager.hpp"
#include "../common.hpp"

CodeView::CodeView(ViewManager &manager) : View(manager, "code")
{
    track.addItem(&text);

    helpLine = buildShortcut(L"c/q", L"quit");
}

vle::Mode
CodeView::buildMode()
{
    vle::Mode mode(getName());
    addTextOperations(mode, text);

    mode.addShortcut({ std::vector<std::wstring> { L"c", L"q" }, [&]() {
        manager.pop();
    }, "return to previous view" });

    return mode;
}

void
CodeView::update()
{
    TermHighlighter hi(*context.node, *context.lang, true, context.node->line);

    std::string printed = hi.print();

    std::vector<cursed::ColorTree> lines;
    for (const boost::string_ref &sr : split(printed, '\n')) {
        lines.push_back(
            cursed::ColorTree::fromEscapeCodes(cursed::toWide(sr.to_string()))
        );
    }
    text.setLines(std::move(lines));
}
