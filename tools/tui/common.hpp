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

#ifndef ZOGRASCOPE_TOOLS_TUI_COMMON_HPP_
#define ZOGRASCOPE_TOOLS_TUI_COMMON_HPP_

#include <functional>

#include "cursed/ListLike.hpp"
#include "cursed/Text.hpp"

#include "vle/Mode.hpp"

// Populates mode with typical list operations allowing to specify callback to
// be invoked on cursor position update.
void addListOperations(vle::Mode &mode, cursed::ListLike &list,
                       std::function<void(cursed::ListLike &)> onPosUpdated);

// Populates mode with typical text operations.
void addTextOperations(vle::Mode &mode, cursed::Text &text);

#endif // ZOGRASCOPE_TOOLS_TUI_COMMON_HPP_
