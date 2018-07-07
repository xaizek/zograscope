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
    groups[+ColorGroup::Deleted].setBackground(Qt::red);
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
    groups[+ColorGroup::Comments].setForeground(Qt::gray);
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
