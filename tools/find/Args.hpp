// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_TOOLS_FIND_ARGS_HPP_
#define ZOGRASCOPE_TOOLS_FIND_ARGS_HPP_

#include "tooling/common.hpp"

// Tool-specific arguments.
struct Args : CommonArgs
{
    bool count; // Only count matches and report statistics.
};

#endif // ZOGRASCOPE_TOOLS_FIND_ARGS_HPP_
