// Copyright (C) 2017 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE__COLORSCHEME_HPP__
#define ZOGRASCOPE__COLORSCHEME_HPP__

#include <array>

#include "colors.hpp"
#include "decoration.hpp"

class ColorScheme
{
public:
    ColorScheme();

public:
    const decor::Decoration & operator[](ColorGroup colorGroup) const;

private:
    std::array<decor::Decoration,
               static_cast<std::size_t>(ColorGroup::ColorGroupCount)> groups;
};

#endif // ZOGRASCOPE__COLORSCHEME_HPP__
