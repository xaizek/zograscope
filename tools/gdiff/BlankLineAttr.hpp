// Copyright (C) 2018 xaizek <xaizek@posteo.net>
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

#ifndef ZOGRASCOPE_TOOLS_GDIFF_BLANKLINEATTR_HPP_
#define ZOGRASCOPE_TOOLS_GDIFF_BLANKLINEATTR_HPP_

#include <QObject>
#include <QTextObjectInterface>

class BlankLineAttr : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit BlankLineAttr(QObject *parent = nullptr);

public:
    static constexpr int getProp() { return QTextFormat::UserProperty + 1; }
    static constexpr int getType() { return QTextFormat::UserObject + 1; }

public:
    virtual QSizeF intrinsicSize(QTextDocument *doc, int posInDocument,
                                 const QTextFormat &format) override;
    virtual void drawObject(QPainter *painter, const QRectF &rect,
                            QTextDocument *doc, int posInDocument,
                            const QTextFormat &format) override;
};

#endif // ZOGRASCOPE_TOOLS_GDIFF_BLANKLINEATTR_HPP_
