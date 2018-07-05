// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__COLORSCHEME_HPP__
#define ZOGRASCOPE__COLORSCHEME_HPP__

#include <array>

#include "decoration.hpp"

enum class ColorGroup
{
    None,

    // Title,

    LineNo,

    // MissingLine,

    // Parts of location entry: <path>:<line>:<col>.
    Path,
    LineNoPart,
    ColNoPart,

    Deleted,
    Inserted,
    Updated,
    Moved,

    Specifiers,
    UserTypes,
    Types,
    Directives,
    Comments,
    Constants,
    Functions,
    Keywords,
    Brackets,
    Operators,

    Other,

    ColorGroupCount
};

class ColorScheme
{
public:
    ColorScheme();

public:
    const decor::Decoration & operator[](ColorGroup colorGroup) const;

private:
    std::array<decor::Decoration, (int)ColorGroup::ColorGroupCount> groups;
};

#endif // ZOGRASCOPE__COLORSCHEME_HPP__
