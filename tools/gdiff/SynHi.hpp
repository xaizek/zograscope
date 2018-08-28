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

#ifndef ZOGRASCOPE__TOOLS__GDIFF__SYNHI_HPP__
#define ZOGRASCOPE__TOOLS__GDIFF__SYNHI_HPP__

#include <vector>

#include <QSyntaxHighlighter>

#include "GuiColorScheme.hpp"

class ColorCane;

class SynHi : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SynHi(QTextDocument *document, std::vector<ColorCane> &&hi);
    ~SynHi();

public:
    const std::vector<ColorCane> & getHi() const;

private:
    virtual void highlightBlock(const QString &text) override;

private:
    std::vector<ColorCane> hi;
    GuiColorScheme cs;
};

#endif // ZOGRASCOPE__TOOLS__GDIFF__SYNHI_HPP__
