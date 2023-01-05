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

#ifndef ZOGRASCOPE_TOOLS_GDIFF_CODEVIEW_HPP_
#define ZOGRASCOPE_TOOLS_GDIFF_CODEVIEW_HPP_

#include <QPlainTextEdit>

#include <vector>

struct StablePos
{
    int line;
    int offset;

    StablePos(int line, int offset);
    explicit StablePos(QTextCursor cursor);

    QTextCursor toCursor(QTextDocument *document) const;
};

bool operator<(const StablePos &a, const StablePos &b);
bool operator==(const StablePos &a, const StablePos &b);

class CodeView : public QPlainTextEdit
{
    Q_OBJECT

    class LineColumn;

public:
    CodeView(QWidget *parent = nullptr);

    using QPlainTextEdit::firstVisibleBlock;

    void setStopPositions(std::vector<StablePos> stopPositions);
    bool goToFirstStopPosition();

signals:
    void scrolled(int pos);
    void focused();

private:
    void updateLineColumn(const QRect &rect, int dy);
    int computeLineColumnWidth();
    void paintLineColumn(QPaintEvent *event);

    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void focusInEvent(QFocusEvent *e) override;

    void goDown();
    void goUp();
    // Scroll current view to put current line at the top.
    void makeTop();
    // Scroll current view to put current line at the bottom.
    void makeBottom();
    void sendKey(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

private:
    LineColumn *lineColumn;
    std::vector<StablePos> positions;
};

#endif // ZOGRASCOPE_TOOLS_GDIFF_CODEVIEW_HPP_
