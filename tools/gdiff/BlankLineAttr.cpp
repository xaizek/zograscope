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

#include "BlankLineAttr.hpp"

#include <QFontMetrics>
#include <QPainter>

BlankLineAttr::BlankLineAttr(QObject *parent) : QObject(parent)
{
}

QSizeF
BlankLineAttr::intrinsicSize(QTextDocument */*doc*/, int /*posInDocument*/,
                             const QTextFormat &format)
{
    auto &tf = static_cast<const QTextCharFormat &>(format);

    QFont fn = tf.font();
    QFontMetrics fm(fn);

    return QSizeF(0, fm.lineSpacing());
}

void
BlankLineAttr::drawObject(QPainter *painter, const QRectF &rect,
                          QTextDocument */*doc*/, int /*posInDocument*/,
                          const QTextFormat &/*format*/)
{
    const int veryWide = 5000;
    QRectF r = rect.adjusted(0, 1, veryWide, 0);

    QPen pen(QColor(0, 0, 0, 0));
    painter->setPen(pen);

    QBrush brush(QColor(0xf0, 0xf0, 0xf0));
    painter->setBrush(brush);

    painter->drawRect(r);

    r = r.adjusted(0, r.height()/2, 0, 0);
    painter->setPen(Qt::DotLine);
    painter->drawLine(r.topLeft(), r.topRight());
}
