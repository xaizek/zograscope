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

#include "ColorScheme.hpp"

// TODO: colors should reside in some configuration file, it's very inconvenient
//       to have to recompile the tool to experiment with coloring.

static inline std::size_t
operator+(ColorGroup cg)
{
    return static_cast<std::size_t>(cg);
}

ColorScheme::ColorScheme()
{
    using namespace decor;
    using namespace decor::literals;

    groups[+ColorGroup::LineNo] = decor::white_bg + decor::black_fg;

    groups[+ColorGroup::Path] = decor::yellow_fg;
    groups[+ColorGroup::LineNoPart] = decor::cyan_fg;
    groups[+ColorGroup::ColNoPart] = decor::cyan_fg;

    groups[+ColorGroup::PieceDeleted] = (124_fg + white_bg + bold + inv)
                                        .prefix("{-"_lit)
                                        .suffix("-}"_lit);
    groups[+ColorGroup::PieceInserted] = (83_fg + inv + black_bg + bold)
                                         .prefix("{+"_lit)
                                         .suffix("+}"_lit);
    groups[+ColorGroup::PieceUpdated] = Decoration(212_bg + 232_fg + bold)
                                        .prefix("{~"_lit)
                                        .suffix("~}"_lit);
    groups[+ColorGroup::UpdatedSurroundings] = (213_bg + 232_fg + bold);

    groups[+ColorGroup::Deleted] = groups[+ColorGroup::PieceDeleted];
    groups[+ColorGroup::Inserted] = groups[+ColorGroup::PieceInserted];
    groups[+ColorGroup::Deleted] = (210_fg + inv + black_bg + bold)
                                   .prefix("{-"_lit)
                                   .suffix("-}"_lit);
    groups[+ColorGroup::Inserted] = (85_fg + inv + black_bg + bold)
                                    .prefix("{+"_lit)
                                    .suffix("+}"_lit);
    groups[+ColorGroup::Updated] = (228_fg + inv + black_bg + bold);
    groups[+ColorGroup::Updated].prefix("{#"_lit).suffix("#}"_lit);
    groups[+ColorGroup::Moved] = (81_fg + inv + bold)
                                 .prefix("{:"_lit)
                                 .suffix(":}"_lit);

    groups[+ColorGroup::Specifiers] = 183_fg;
    groups[+ColorGroup::UserTypes] = 215_fg;
    groups[+ColorGroup::Types] = 85_fg;
    groups[+ColorGroup::Directives] = 228_fg;
    groups[+ColorGroup::Comments] = 248_fg;
    groups[+ColorGroup::Functions] = 81_fg;
    groups[+ColorGroup::Keywords] = 115_fg;
    groups[+ColorGroup::Brackets] = 222_fg;
    groups[+ColorGroup::Operators] = 224_fg;
    groups[+ColorGroup::Constants] = 219_fg;
}

const decor::Decoration &
ColorScheme::operator[](ColorGroup colorGroup) const
{
    return groups[+colorGroup];
}
