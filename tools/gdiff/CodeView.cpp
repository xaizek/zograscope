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

#include "CodeView.hpp"

#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

#include <cassert>

#include <algorithm>
#include <utility>
#include <vector>

#include "utils/nums.hpp"

class CodeView::LineColumn : public QWidget
{
public:
    static constexpr int leftMargin = 2;
    static constexpr int rightMargin = 3;

public:
    LineColumn(CodeView *parent) : QWidget(parent), parent(parent) { }

private:
    virtual QSize sizeHint() const override {
        return { parent->computeLineColumnWidth(), 0 };
    }

    virtual void paintEvent(QPaintEvent *event) override {
        parent->paintLineColumn(event);
    }

    virtual void wheelEvent(QWheelEvent *e) override {
        parent->wheelEvent(e);
    }

private:
    CodeView *parent;
};

StablePos::StablePos(int line, int offset) : line(line), offset(offset)
{
}

StablePos::StablePos(QTextCursor cursor)
{
    QTextDocument *document = cursor.document();
    line = -1;
    offset = cursor.position();
    for (QTextBlock block = cursor.block(); ; block = block.previous()) {
        if (block.userState() >= 0) {
            line = block.userState();
            offset -= block.position();
            break;
        }

        if (block == document->begin()) {
            break;
        }
    }
}

QTextCursor
StablePos::toCursor(QTextDocument *document) const
{
    for (QTextBlock block = document->begin();
         block != document->end();
         block = block.next()) {
        if (block.userState() == line) {
            QTextCursor c(document);
            c.setPosition(offset + block.position());
            return c;
        }
    }
    return QTextCursor(document);
}

bool
operator<(const StablePos &a, const StablePos &b)
{
    return (a.line < b.line || (a.line == b.line && a.offset < b.offset));
}

bool
operator==(const StablePos &a, const StablePos &b)
{
    return (a.line == b.line && a.offset == b.offset);
}

CodeView::CodeView(QWidget *parent)
    : QPlainTextEdit(parent), lineColumn(new LineColumn(this))
{
    connect(this, &QPlainTextEdit::blockCountChanged, [this]() {
        setViewportMargins(computeLineColumnWidth(), 0, 0, 0);
    });
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeView::updateLineColumn);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            [&](int pos) { emit scrolled(pos); });
    // Qt seems to have a bug or just very weird behaviour of not emitting
    // QScrollBar::valueChanged when scrollbar is updated as a result of a key
    // press.  Below is a workaround.
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            [&]() { emit scrolled(verticalScrollBar()->sliderPosition()); });
}

void
CodeView::setStopPositions(std::vector<StablePos> stopPositions)
{
    assert(std::is_sorted(stopPositions.cbegin(), stopPositions.cend()) &&
           "Positions must be sorted!");
    positions = std::move(stopPositions);
}

bool
CodeView::goToFirstStopPosition()
{
    if (positions.empty()) return false;

    setTextCursor(positions.front().toCursor(document()));
    return true;
}

void
CodeView::updateLineColumn(const QRect &rect, int dy)
{
    if (dy) {
        lineColumn->scroll(0, dy);
    } else {
        lineColumn->update(0, rect.y(), lineColumn->width(), rect.height());
    }
}

int
CodeView::computeLineColumnWidth()
{
    return LineColumn::leftMargin
         + fontMetrics().width('0')*countWidth(blockCount())
         + LineColumn::rightMargin;
}

void
CodeView::paintLineColumn(QPaintEvent *event)
{
    QPainter painter(lineColumn);
    painter.fillRect(event->rect(), QColor(Qt::gray).light(148));
    painter.setPen(Qt::black);

    QFont normalFont = painter.font();

    QTextBlock block = firstVisibleBlock();
    int top = contentOffset().y() + blockBoundingRect(block).top();
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    const int from = event->rect().top();
    const int to = event->rect().bottom();
    while (block.isValid() && top <= to) {
        int lineNum = block.userState();

        if (bottom >= from && block.isVisible() && lineNum >= 0) {
            if (block == textCursor().block()) {
                QFont boldFont = normalFont;
                boldFont.setWeight(QFont::Bold);
                painter.setFont(boldFont);
            }
            painter.drawText(0,
                             top,
                             lineColumn->width() - LineColumn::rightMargin,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             QString::number(lineNum + 1));
            if (block == textCursor().block()) {
                painter.setFont(normalFont);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
    }
}

void
CodeView::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect r = contentsRect();
    lineColumn->setGeometry({ r.left(), r.top(),
                              computeLineColumnWidth(), r.height() });
}

void
CodeView::keyPressEvent(QKeyEvent *e)
{
    if (e->text() == "z") {
        centerCursor();
        emit scrolled(verticalScrollBar()->sliderPosition());
        return;
    }
    if (e->text() == "j") {
        return goDown();
    }
    if (e->text() == "k") {
        return goUp();
    }
    if (e->text() == "t") {
        return makeTop();
    }
    if (e->text() == "b") {
        return makeBottom();
    }
    if (e->text() == "g") {
        return sendKey(Qt::Key_Home, Qt::ControlModifier);
    }
    if (e->text() == "G") {
        return sendKey(Qt::Key_End, Qt::ControlModifier);
    }
    if (e->key() == Qt::Key_E && e->modifiers() == Qt::ControlModifier) {
        int newPos = verticalScrollBar()->sliderPosition() + 1;
        verticalScrollBar()->setSliderPosition(newPos);
        return;
    }
    if (e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier) {
        int newPos = verticalScrollBar()->sliderPosition() - 1;
        verticalScrollBar()->setSliderPosition(newPos);
        return;
    }
    return QPlainTextEdit::keyPressEvent(e);
}

void
CodeView::focusInEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusInEvent(e);
    emit focused();
}

void
CodeView::goDown()
{
    StablePos pos(textCursor());
    auto it = std::upper_bound(positions.cbegin(), positions.cend(), pos);
    if (it != positions.cend() && *it == pos) ++it;
    if (it == positions.cend()) return;

    setTextCursor(it->toCursor(document()));
}

void
CodeView::goUp()
{
    StablePos pos(textCursor());
    auto it = std::lower_bound(positions.cbegin(), positions.cend(), pos);
    if (it == positions.cbegin()) return;

    setTextCursor((--it)->toCursor(document()));
}

void
CodeView::makeTop()
{
    int newPos = textCursor().block().blockNumber();
    verticalScrollBar()->setSliderPosition(newPos);
    setTextCursor(textCursor());
    emit scrolled(verticalScrollBar()->sliderPosition());
}

void
CodeView::makeBottom()
{
    QTextCursor bottom = cursorForPosition(QPoint(0.0f, viewport()->height()));
    int height = bottom.block().blockNumber()
        - firstVisibleBlock().blockNumber() + 1;
    int newPos = textCursor().block().blockNumber() - height + 1;
    verticalScrollBar()->setSliderPosition(newPos);
    setTextCursor(textCursor());
    emit scrolled(verticalScrollBar()->sliderPosition());
}

void
CodeView::sendKey(int key, Qt::KeyboardModifiers modifiers)
{
    QKeyEvent ev(QEvent::KeyPress, key, modifiers);
    QPlainTextEdit::keyPressEvent(&ev);
}
