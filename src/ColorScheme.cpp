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

    groups[+ColorGroup::Deleted] = (210_fg + inv + black_bg + bold)
                                   .prefix("{-"_lit)
                                   .suffix("-}"_lit);
    groups[+ColorGroup::Inserted] = (85_fg + inv + black_bg + bold)
                                    .prefix("{+"_lit)
                                    .suffix("+}"_lit);
    groups[+ColorGroup::Updated] = (228_fg + inv + black_bg + bold)
                                   .prefix("{#"_lit)
                                   .suffix("#}"_lit);
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
