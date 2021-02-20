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

#include "common.hpp"

#include "cursed/ListLike.hpp"
#include "cursed/Text.hpp"

#include "vle/Mode.hpp"

void
addListOperations(vle::Mode &mode, cursed::ListLike &list,
                  std::function<void(cursed::ListLike &)> onPosUpdated)
{
    mode.addShortcut({ L"G", [&,onPosUpdated](int count) {
        if (count < 0) {
            list.moveToLast();
        } else {
            list.moveToPos(count - 1);
        }
        onPosUpdated(list);
    }, "go to the last or [count]-th item" });
    mode.addShortcut({ L"gg", [&,onPosUpdated](int count) {
        if (count < 0) {
            list.moveToFirst();
        } else {
            list.moveToPos(count - 1);
        }
        onPosUpdated(list);
    }, "go to the first or [count]-th item" });
    mode.addShortcut({ L"j", [&,onPosUpdated](int count) {
        list.moveDown(count < 0 ? 1 : count);
        onPosUpdated(list);
    }, "go [count] items below" });
    mode.addShortcut({ L"k", [&,onPosUpdated](int count) {
        list.moveUp(count < 0 ? 1 : count);
        onPosUpdated(list);
    }, "go [count] items above" });
}

void
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
