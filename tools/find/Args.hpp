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

#ifndef ZOGRASCOPE__TOOLS__FIND__ARGS_HPP__
#define ZOGRASCOPE__TOOLS__FIND__ARGS_HPP__

#include "tooling/common.hpp"

// Tool-specific arguments.
struct Args : CommonArgs
{
    bool count; // Only count matches and report statistics.
};

#endif // ZOGRASCOPE__TOOLS__FIND__ARGS_HPP__
