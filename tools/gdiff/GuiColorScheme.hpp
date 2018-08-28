#ifndef GUICOLORSCHEME_HPP
#define GUICOLORSCHEME_HPP

#include <array>

#include <QTextCharFormat>

#include "ColorScheme.hpp"

class GuiColorScheme
{
public:
    GuiColorScheme();

public:
    const QTextCharFormat & operator[](ColorGroup colorGroup) const;

private:
    std::array<QTextCharFormat,
               static_cast<std::size_t>(ColorGroup::ColorGroupCount)> groups;
};

#endif // GUICOLORSCHEME_HPP
