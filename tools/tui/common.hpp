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

#ifndef ZOGRASCOPE__TOOLS__TUI__COMMON_HPP__
#define ZOGRASCOPE__TOOLS__TUI__COMMON_HPP__

#include "cursed/ListLike.hpp"
#include "cursed/Text.hpp"

#include "vle/Mode.hpp"

// Populates mode with typical list operations.
void addListOperations(vle::Mode &mode, cursed::ListLike &list);

// Populates mode with typical text operations.
void addTextOperations(vle::Mode &mode, cursed::Text &text);

#endif // ZOGRASCOPE__TOOLS__TUI__COMMON_HPP__
