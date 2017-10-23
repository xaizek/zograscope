#ifndef COLORSCHEME_HPP__
#define COLORSCHEME_HPP__

#include <array>

#include "decoration.hpp"

enum class ColorGroup
{
    // Title,

    // LineNo,

    // MissingLine,

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

#endif // COLORSCHEME_HPP__
