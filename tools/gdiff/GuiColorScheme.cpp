// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#include "GuiColorScheme.hpp"

static inline std::size_t
operator+(ColorGroup cg)
{
    return static_cast<std::size_t>(cg);
}

GuiColorScheme::GuiColorScheme()
{
    groups[+ColorGroup::Inserted].setBackground(Qt::green);
    groups[+ColorGroup::Deleted].setForeground(Qt::white);
    groups[+ColorGroup::Deleted].setBackground(QColor(Qt::red).lighter(120));
    groups[+ColorGroup::Updated].setBackground(Qt::yellow);
    groups[+ColorGroup::Moved].setForeground(Qt::white);
    groups[+ColorGroup::Moved].setBackground(Qt::blue);

    groups[+ColorGroup::PieceInserted].setBackground(QColor(Qt::yellow).light(150));
    groups[+ColorGroup::PieceInserted].setForeground(QColor(Qt::green).dark(200));
    groups[+ColorGroup::PieceInserted].setFontWeight(QFont::Bold);
    groups[+ColorGroup::PieceDeleted].setBackground(QColor(Qt::yellow).light(150));
    groups[+ColorGroup::PieceDeleted].setForeground(Qt::red);
    groups[+ColorGroup::PieceDeleted].setFontWeight(QFont::Bold);
    groups[+ColorGroup::PieceUpdated].setBackground(Qt::yellow);
    groups[+ColorGroup::PieceUpdated].setFontWeight(QFont::Bold);

    groups[+ColorGroup::Specifiers].setForeground(QColor(Qt::yellow).darker(250));
    groups[+ColorGroup::UserTypes].setForeground(Qt::magenta);
    groups[+ColorGroup::Types].setForeground(QColor(Qt::green).dark(170));
    groups[+ColorGroup::Types].setFontWeight(QFont::Bold);
    groups[+ColorGroup::Directives].setForeground(Qt::darkYellow);
    groups[+ColorGroup::Comments].setForeground(QColor(Qt::gray).darker());
    groups[+ColorGroup::Functions].setForeground(Qt::blue);
    groups[+ColorGroup::Keywords].setFontWeight(QFont::Bold);
    //groups[+ColorGroup::Brackets] = 222_fg;
    //groups[+ColorGroup::Operators] = 224_fg;
    groups[+ColorGroup::Constants].setForeground(Qt::magenta);
}

const QTextCharFormat &
GuiColorScheme::operator[](ColorGroup colorGroup) const
{
    return groups[+colorGroup];
}
