#ifndef ZOGRASCOPE__TOOLS__GDIFF__GUICOLORSCHEME_HPP__
#define ZOGRASCOPE__TOOLS__GDIFF__GUICOLORSCHEME_HPP__

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

#endif // ZOGRASCOPE__TOOLS__GDIFF__GUICOLORSCHEME_HPP__
