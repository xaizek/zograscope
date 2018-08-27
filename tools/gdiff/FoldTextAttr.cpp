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

#include "FoldTextAttr.hpp"

#include <QFontMetrics>
#include <QPainter>

FoldTextAttr::FoldTextAttr(QObject *parent) : QObject(parent)
{ }

QSizeF
FoldTextAttr::intrinsicSize(QTextDocument */*doc*/, int /*posInDocument*/,
                            const QTextFormat &format)
{
    auto &tf = static_cast<const QTextCharFormat&>(format);

    QFont fn = tf.font();
    QFontMetrics fm(fn);

    return QSizeF(0, fm.lineSpacing());
}

void
FoldTextAttr::drawObject(QPainter *painter, const QRectF &rect,
                         QTextDocument */*doc*/, int /*posInDocument*/,
                         const QTextFormat &format)
{
    const int veryWide = 5000;
    QRectF r = rect.adjusted(0, 0, veryWide, 0);

    QVariant v = format.property(getProp());
    auto nLines = v.value<int>();

    QPen pen(QColor(0, 0, 0, 0));
    painter->setPen(pen);

    QBrush brush(QColor(0xe8, 0xe8, 0x58)/*, Qt::Dense7Pattern*/);
    painter->setBrush(brush);

    painter->drawRect(r);

    painter->setPen(QPen(QColor(0, 0, 0)));

    QFont boldFont = painter->font();
    boldFont.setWeight(QFont::Bold);
    painter->setFont(boldFont);

    QString s("  folded %1 lines");
    painter->drawText(r.adjusted(0, 1, 0, 1), s.arg(nLines),
                      Qt::AlignLeft | Qt::AlignBottom);
}
