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

#include "ZSDiff.hpp"

#include <QList>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextStream>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTextBrowser>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>

#include "pmr/monolithic.hpp"

#include "tooling/common.hpp"
#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "ColorCane.hpp"
#include "DiffList.hpp"
#include "Highlighter.hpp"
#include "TreeBuilder.hpp"
#include "Language.hpp"
#include "STree.hpp"
#include "align.hpp"
#include "colors.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "CodeView.hpp"
#include "SynHi.hpp"
#include "ui_zsdiff.h"

// TODO: diff entries shouldn't be diffed more than once, we need to store
//       results of diffing along with state of code views (maybe just be
//       creating N pairs of widgets and switching to them as needed).

// TODO: probably need to display list of diff entries in the GUI somehow.

// TODO: fallback to regular diff when parsing fails?

// TODO: in future we could support more modes of `git diff`, `git show` and
//       `git stash` invocation.

struct ZSDiff::SideInfo
{
    std::vector<ColorCane> hi;
    std::vector<std::map<int, TokenInfo *>> map;
};

// Helper for filling code view quickly (doing it without buffering is awfully
// slow).
class ZSDiff::CodeBuffer
{
public:
    // Adds a blank line.
    void addBlank()
    {
        text += QChar::ObjectReplacementCharacter;
        text += '\n';
        userState.push_back(-1);
        visible.push_back(true);
        format.emplace_back();
        format.back().setObjectType(BlankLineAttr::getType());
    }

    // Adds line with the text.
    void addLine(boost::string_ref s, int lineNo)
    {
        text += QByteArray(s.data(), s.size());
        text += '\n';
        userState.push_back(lineNo);
        visible.push_back(true);
        format.emplace_back();
    }

    // Adds a fold of specified height.
    void addFold(int height)
    {
        QTextCharFormat foldTextFormat;
        foldTextFormat.setObjectType(FoldTextAttr::getType());

        QVariant v;
        v.setValue(height);
        foldTextFormat.setProperty(FoldTextAttr::getProp(), v);

        text += QChar::ObjectReplacementCharacter;
        text += '\n';
        userState.push_back(-2);
        visible.push_back(false);
        format.emplace_back(std::move(foldTextFormat));
    }

    // Displays buffered data onto a code view.
    void display(CodeView *view)
    {
        if (text.endsWith('\n')) {
            text.remove(text.size() - 1, 1);
        }

        QTextDocument *doc = view->document();
        doc->setPlainText(text);

        QTextCharFormat noFormat;

        int i = 0;
        for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
            if (!b.isValid()) {
                continue;
            }

            b.setUserState(userState[i]);
            b.setVisible(visible[i]);

            if (format[i] != noFormat) {
                QTextCursor c(b);
                c.setPosition(c.position() + 1, QTextCursor::KeepAnchor);
                c.setCharFormat(format[i]);
            }

            ++i;
        }
    }

private:
    QString text;                        // Complete text.
    std::vector<int> userState;          // User state of corresponding line.
    std::vector<bool> visible;           // Whether specific line is visible.
    std::vector<QTextCharFormat> format; // Format of corresponding line.
};


Q_DECLARE_METATYPE(TokenInfo *)

ZSDiff::SideInfo
ZSDiff::printTree(Tree &tree, CodeView *textEdit, bool original)
{
    std::vector<StablePos> stopPositions;

    tree.propagateStates();
    Highlighter highlighter(tree, original);
    highlighter.setPrintBrackets(false);
    highlighter.setTransparentDiffables(false);

    std::vector<ColorCane> hi = highlighter.print().splitIntoLines();

    std::vector<std::map<int, TokenInfo *>> map(hi.size());

    int line = 0;
    for (const ColorCane &cc : hi) {
        int lineFrom = 0;
        bool positionedOnThisLine = false;
        for (const ColorCanePiece &piece : cc) {
            if (piece.node != nullptr &&
                (piece.node->state != State::Unchanged || piece.node->moved) &&
                !positionedOnThisLine) {
                stopPositions.emplace_back(line, lineFrom);
                positionedOnThisLine = true;
            }

            int lineTo = lineFrom + piece.text.length();
            if (piece.node != nullptr && piece.node->relative != nullptr) {
                TokenInfo *in = &info[original ? piece.node
                                               : piece.node->relative];
                map[line].emplace(lineTo, in);

                if (original) {
                    if (in->oldTo.offset == 0) {
                        in->oldFrom.line = line;
                        in->oldFrom.offset = lineFrom;
                    }
                    in->oldTo.offset = lineTo;
                    in->oldTo.line = line;
                } else {
                    if (in->newTo.offset == 0) {
                        in->newFrom.line = line;
                        in->newFrom.offset = lineFrom;
                    }
                    in->newTo.offset = lineTo;
                    in->newTo.line = line;
                }
            } else {
                map[line].emplace(lineTo, nullptr);
            }
            lineFrom = lineTo;
        }
        ++line;
    }

    textEdit->setStopPositions(std::move(stopPositions));

    return { std::move(hi), std::move(map) };
}

ZSDiff::ZSDiff(LaunchMode launchMode, DiffList diffList, TimeReport &tr,
               QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ZSDiff),
      loaded(false),
      scrollDiff(0),
      syncScrolls(true),
      syncMatches(false),
      firstTimeFocus(true),
      folded(false),
      oldTree(&mr),
      newTree(&mr),
      timeReport(tr),
      launchMode(launchMode),
      diffList(std::move(diffList))
{
    ui->setupUi(this);

    qApp->installEventFilter(this);

    ui->oldCode->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                         Qt::TextSelectableByKeyboard);
    ui->oldCode->document()->setDocumentMargin(1);
    ui->newCode->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                         Qt::TextSelectableByKeyboard);
    ui->newCode->document()->setDocumentMargin(1);

    ui->mainToolBar->hide();
    ui->statusBar->showMessage("Press F1 to toggle help");

    QFile file(":/help.html");
    if (file.open(QFile::ReadOnly)) {
        ui->helpTextBrowser->setHtml(file.readAll());
    } else {
        ui->helpTextBrowser->setText("Loading help failed");
    }

    auto onPosChanged = [&](QPlainTextEdit *textEdit) {
        if (!loaded) return;
        QTextCursor cursor = textEdit->textCursor();
        if (cursor.hasSelection()) return;
        highlightMatch(textEdit);
    };
    connect(ui->oldCode, &QPlainTextEdit::cursorPositionChanged,
            [=]() { onPosChanged(ui->oldCode); });
    connect(ui->newCode, &QPlainTextEdit::cursorPositionChanged,
            [=]() { onPosChanged(ui->newCode); });

    auto onScrolled = [&](CodeView *codeView) {
        if (loaded && syncScrolls) {
            syncScrollTo(codeView);
        }
    };
    connect(ui->oldCode, &CodeView::scrolled, [=](int /*pos*/) {
        onScrolled(ui->oldCode);
    });
    connect(ui->newCode, &CodeView::scrolled, [=](int /*pos*/) {
        onScrolled(ui->newCode);
    });

    auto onFocus = [&](CodeView *view) {
        if (!loaded) return;
        if (firstTimeFocus) {
            firstTimeFocus = false;
            view->centerCursor();
            syncScrollTo(view);
        }
        syncOtherCursor(otherView(view));
        highlightMatch(view);
    };
    connect(ui->oldCode, &CodeView::focused, [=]() { onFocus(ui->oldCode); });
    connect(ui->newCode, &CodeView::focused, [=]() { onFocus(ui->newCode); });

    loadDiff(this->diffList.getCurrent());
}

void
ZSDiff::loadDiff(const DiffEntry &diffEntry)
{
    auto timer = timeReport.measure("loading-entry");

    loaded = false;

    ui->oldLabel->setText(QString("--- %1").arg(diffEntry.original.title.c_str()));
    ui->newLabel->setText(QString("+++ %1").arg(diffEntry.updated.title.c_str()));

    updateTitle();

    if (optional_t<Tree> &&tree = buildTreeFromFile(diffEntry.original.path,
                                                    diffEntry.original.contents,
                                                    {}, timeReport, &mr)) {
        oldTree = *tree;
    } else {
        ui->oldCode->setPlaceholderText("   PARSING HAS FAILED");
        ui->newCode->setPlaceholderText("   PARSING HAS FAILED");
        return;
    }

    if (optional_t<Tree> &&tree = buildTreeFromFile(diffEntry.updated.path,
                                                    diffEntry.updated.contents,
                                                    {}, timeReport, &mr)) {
        newTree = *tree;
    } else {
        ui->oldCode->setPlaceholderText("   PARSING HAS FAILED");
        ui->newCode->setPlaceholderText("   PARSING HAS FAILED");
        return;
    }

    compare(oldTree, newTree, timeReport, true, false);

    QTextDocument *oldDoc = ui->oldCode->document();
    QTextDocument *newDoc = ui->newCode->document();

    SideInfo leftSide = printTree(oldTree, ui->oldCode, true);
    oldMap = std::move(leftSide.map);
    SideInfo rightSide = printTree(newTree, ui->newCode, false);
    newMap = std::move(rightSide.map);

    oldDoc->documentLayout()->registerHandler(blankLineAttr.getType(),
                                              &blankLineAttr);
    oldDoc->documentLayout()->registerHandler(foldTextAttr.getType(),
                                              &foldTextAttr);
    newDoc->documentLayout()->registerHandler(blankLineAttr.getType(),
                                              &blankLineAttr);
    newDoc->documentLayout()->registerHandler(foldTextAttr.getType(),
                                              &foldTextAttr);

    oldSynHi.reset();
    newSynHi.reset();

    diffAndPrint(timeReport);

    oldSynHi.reset(new SynHi(oldDoc, std::move(leftSide.hi)));
    newSynHi.reset(new SynHi(newDoc, std::move(rightSide.hi)));

    fold();

    ui->newCode->moveCursor(QTextCursor::Start);
    ui->oldCode->moveCursor(QTextCursor::Start);

    loaded = true;

    // Navigate to first change in old or new version of the code and highlight
    // current line.
    if (ui->oldCode->goToFirstStopPosition()) {
        ui->oldCode->setFocus();
    } else {
        ui->newCode->goToFirstStopPosition();
        ui->newCode->setFocus();
    }
}

void
ZSDiff::updateTitle()
{
    QString title;
    switch (launchMode) {
        case LaunchMode::Standalone:
            title = "Standalone mode";
            break;
        case LaunchMode::GitExt:
            title = "Git external diff mode";
            break;
        case LaunchMode::Staged:
            title = QString("Staged in repository (%1/%2)")
                        .arg(diffList.getPosition())
                        .arg(diffList.getCount());
            break;
        case LaunchMode::Unstaged:
            title = QString("Unstaged in repository (%1/%2)")
                        .arg(diffList.getPosition())
                        .arg(diffList.getCount());;
            break;
    }
    ui->title->setText(title);
}

void
ZSDiff::diffAndPrint(TimeReport &tr)
{
    auto timer = tr.measure("aligning-and-printing");

    QTextCharFormat blankLineFormat;
    blankLineFormat.setObjectType(blankLineAttr.getType());

    QTextCharFormat foldTextFormat;
    foldTextFormat.setObjectType(foldTextAttr.getType());

    DiffSource lsrc = (tr.measure("left-print"),
                       DiffSource(*oldTree.getRoot()));
    DiffSource rsrc = (tr.measure("right-print"),
                       DiffSource(*newTree.getRoot()));
    std::vector<DiffLine> diff = (tr.measure("align"),
                                  makeDiff(std::move(lsrc), std::move(rsrc)));

    timer.measure("visualizing");

    leftFolded.assign(lsrc.lines.size(), false);
    rightFolded.assign(rsrc.lines.size(), false);

    CodeBuffer leftBuf, rightBuf;

    unsigned int i = 0U;
    unsigned int j = 0U;
    for (DiffLine d : diff) {
        switch (d.type) {
            case Diff::Left:
                leftBuf.addLine(lsrc.lines[i].text.str(), i);
                rightBuf.addBlank();
                ++i;
                break;
            case Diff::Right:
                leftBuf.addBlank();
                rightBuf.addLine(rsrc.lines[j].text.str(), j);
                ++j;
                break;
            case Diff::Identical:
            case Diff::Different:
                leftBuf.addLine(lsrc.lines[i].text.str(), i);
                rightBuf.addLine(rsrc.lines[j].text.str(), j);
                ++i;
                ++j;
                break;

            case Diff::Fold:
                leftBuf.addFold(d.data);
                rightBuf.addFold(d.data);
                for (int k = 0; k < d.data; ++k, ++i, ++j) {
                    leftBuf.addLine(lsrc.lines[i].text.str(), i);
                    rightBuf.addLine(rsrc.lines[j].text.str(), j);
                    leftFolded[i] = true;
                    rightFolded[j] = true;
                }
                break;
        }
    }

    timer.measure("displaying");

    leftBuf.display(ui->oldCode);
    rightBuf.display(ui->newCode);
}

void
ZSDiff::highlightMatch(QPlainTextEdit *textEdit)
{
    QColor currColor(0xa0, 0xe0, 0xa0, 0x60);
    QColor otherColor(0xa0, 0xa0, 0xe0, 0x60);
    QColor otherMatchColor(0xff, 0x80, 0x80, 0x60);

    QTextCharFormat oldLineFormat;
    QColor lineColor = (ui->oldCode->hasFocus() ? currColor : otherColor);
    oldLineFormat.setBackground(lineColor);
    oldLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    QTextCharFormat newLineFormat;
    lineColor = (ui->newCode->hasFocus() ? currColor : otherColor);
    newLineFormat.setBackground(lineColor);
    newLineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    auto collectFormats = [&](const ColorCane &cc, QPlainTextEdit *textEdit,
                              int position,
                            QList<QTextEdit::ExtraSelection> &extraSelections) {
        int from = position;
        for (const ColorCanePiece &piece : cc) {
            const QTextCharFormat &f = cs[piece.hi];
            const int to = from + piece.text.length();
            if (f.foreground() != QBrush() || f.background() != QBrush()) {
                QTextCursor cursor(textEdit->document());
                cursor.setPosition(from);
                cursor.setPosition(to, QTextCursor::KeepAnchor);
                extraSelections.append({ cursor, cs[piece.hi] });
            }
            from = to;
        }
    };

    QList<QTextEdit::ExtraSelection> newExtraSelections, oldExtraSelections;

    int rightLine = ui->newCode->textCursor().block().userState();
    if (rightLine >= 0) {
        collectFormats(newSynHi->getHi()[rightLine],
                       ui->newCode,
                       ui->newCode->textCursor().block().position(),
                       newExtraSelections);
    }

    int leftLine = ui->oldCode->textCursor().block().userState();
    if (leftLine >= 0) {
        collectFormats(oldSynHi->getHi()[leftLine],
                       ui->oldCode,
                       ui->oldCode->textCursor().block().position(),
                       oldExtraSelections);
    }

    newExtraSelections.append({ ui->newCode->textCursor(), newLineFormat });
    oldExtraSelections.append({ ui->oldCode->textCursor(), oldLineFormat });

    TokenInfo *i = getTokenInfo(textEdit);
    if (i == nullptr) {
        ui->newCode->setExtraSelections(newExtraSelections);
        ui->oldCode->setExtraSelections(oldExtraSelections);
        return;
    }

    if (textEdit == ui->oldCode) {
        newLineFormat.setBackground(otherMatchColor);
    } else {
        oldLineFormat.setBackground(otherMatchColor);
    }

    QTextCharFormat format;
    format.setFontOverline(true);
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    syncScrolls = false;

    auto updateCursor = [&](QPlainTextEdit *textEdit,
                            QList<QTextEdit::ExtraSelection> &extraSelections,
                            StablePos fromPos, StablePos toPos) {
        int from, to;
        resolveRange(textEdit, fromPos, toPos, from, to);

        QTextCursor sel(textEdit->document());
        sel.setPosition(from);
        sel.setPosition(to, QTextCursor::KeepAnchor);

        QTextCursor lineCursor(textEdit->document());
        lineCursor.setPosition(from);

        int line = lineCursor.block().userState();
        if (line >= 0) {
            if (textEdit == ui->oldCode) {
                collectFormats(oldSynHi->getHi()[line], textEdit,
                               lineCursor.block().position(), extraSelections);
            } else {
                collectFormats(newSynHi->getHi()[line], textEdit,
                               lineCursor.block().position(), extraSelections);
            }
        }
        extraSelections.append({ sel, format });
        extraSelections.append({ lineCursor, textEdit == ui->oldCode
                                             ? oldLineFormat
                                             : newLineFormat });
        textEdit->setExtraSelections(extraSelections);
    };

    updateCursor(ui->newCode, newExtraSelections, i->newFrom, i->newTo);
    updateCursor(ui->oldCode, oldExtraSelections, i->oldFrom, i->oldTo);

    syncScrolls = true;

    ui->oldCode->setTextCursor(ui->oldCode->textCursor());
    ui->newCode->setTextCursor(ui->newCode->textCursor());

    if (syncMatches) {
        alignViews();
    }

    ui->newCode->setExtraSelections(newExtraSelections);
    ui->oldCode->setExtraSelections(oldExtraSelections);
}

void
ZSDiff::syncOtherCursor(QPlainTextEdit *textEdit)
{
    struct CursInfo {
        QTextCursor cursor;
        bool needUpdate;
    };

    TokenInfo *i = getTokenInfo(textEdit);
    if (i == nullptr) {
        return;
    }

    auto updateCursor = [&](QPlainTextEdit *textEdit,
                            StablePos fromPos, StablePos toPos) {
        int from, to;
        resolveRange(textEdit, fromPos, toPos, from, to);

        QTextCursor cursor = textEdit->textCursor();
        int currentPos = cursor.position();
        cursor.setPosition(from);
        return CursInfo { cursor, (currentPos < from || currentPos > to) };
    };

    CursInfo newInfo = updateCursor(ui->newCode, i->newFrom, i->newTo);
    CursInfo oldInfo = updateCursor(ui->oldCode, i->oldFrom, i->oldTo);

    if (oldInfo.needUpdate) {
        ui->oldCode->setTextCursor(oldInfo.cursor);
    }
    if (newInfo.needUpdate) {
        ui->newCode->setTextCursor(newInfo.cursor);
    }
}

void
ZSDiff::resolveRange(QPlainTextEdit *textEdit,
                     StablePos fromPos, StablePos toPos,
                     int &from, int &to)
{
    from = fromPos.offset;
    to = toPos.offset;
    for (QTextBlock block = textEdit->document()->begin();
         block != textEdit->document()->end();
         block = block.next()) {
        if (block.userState() == fromPos.line) {
            from += block.position();
        }
        if (block.userState() == toPos.line) {
            to += block.position();
            break;
        }
    }
}

TokenInfo *
ZSDiff::getTokenInfo(QPlainTextEdit *textEdit)
{
    std::vector<std::map<int, TokenInfo *>> &map = (textEdit == ui->oldCode)
                                                 ? oldMap
                                                 : newMap;
    int lineNum = textEdit->textCursor().block().userState();
    const std::map<int, TokenInfo *> &line = lineNum >= 0
                                           ? map[lineNum]
                                           : std::map<int, TokenInfo *>();
    auto it = line.upper_bound(textEdit->textCursor().positionInBlock());
    return (it == line.end() ? nullptr : it->second);
}

void
ZSDiff::fold()
{
    auto foldView = [this](CodeView *view, const std::vector<bool> &folded) {
        QTextDocument *doc = view->document();

        for (QTextBlock block = doc->begin();
            block != doc->end();
            block = block.next()) {
            int line = block.userState();
            if (line < 0) {
                if (line == -2) {
                    block.setVisible(true);
                }
                continue;
            }

            if (folded[line]) {
                block.setVisible(false);
            }
        }

        updateView(view);
    };

    foldView(ui->oldCode, leftFolded);
    foldView(ui->newCode, rightFolded);
    updateLayout();
    folded = true;
}

void
ZSDiff::unfold()
{
    auto unfoldView = [this](CodeView *view) {
        QTextDocument *doc = view->document();

        for (QTextBlock block = doc->begin();
            block != doc->end();
            block = block.next()) {
            int line = block.userState();
            if (line < 0) {
                if (line == -2) {
                    block.setVisible(false);
                }
                continue;
            }

            if (!block.isVisible()) {
                block.setVisible(true);
            }
        }

        updateView(view);
    };

    unfoldView(ui->oldCode);
    unfoldView(ui->newCode);
    updateLayout();
    folded = false;
}

void
ZSDiff::updateView(CodeView *view)
{
    view->update();
    view->updateGeometry();
    view->viewport()->update();
    view->verticalScrollBar()->update();
}

void
ZSDiff::updateLayout()
{
    if (ui->splitter->orientation() == Qt::Vertical) {
        ui->splitter->setOrientation(Qt::Horizontal);
        ui->splitter->setOrientation(Qt::Vertical);
    } else {
        ui->splitter->setOrientation(Qt::Vertical);
        ui->splitter->setOrientation(Qt::Horizontal);
    }

    CodeView *view = activeView();
    view->ensureCursorVisible();
    highlightMatch(view);
}

ZSDiff::~ZSDiff()
{
    delete ui;
}

Q_DECLARE_METATYPE(QTextDocumentFragment)

void
ZSDiff::switchView()
{
    CodeView *view = inactiveView();
    view->setFocus();
    if (onlyMode()) {
        if (view == ui->oldCode) {
            ui->splitter->setSizes({ 1, 0 });
        } else {
            ui->splitter->setSizes({ 0, 1 });
        }
    }
    syncOtherCursor(otherView(view));
    highlightMatch(view);
}

bool
ZSDiff::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) {
        return QObject::eventFilter(obj, event);
    }

    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->text() == "q") {
        close();
    } else if (keyEvent->text() == "a") {
        doSyncMatches();
    } else if (keyEvent->text() == "A") {
        // Reset token alignment.
        scrollDiff = 0;
        syncScrollTo(activeView());
    } else if (keyEvent->text() == "f") {
        if (folded) {
            unfold();
        } else {
            fold();
        }
    } else if (keyEvent->text() == "s") {
        split(Qt::Vertical);
    } else if (keyEvent->text() == "v") {
        split(Qt::Horizontal);
    } else if (keyEvent->text() == "o") {
        if (!onlyMode() && ui->oldCode->hasFocus()) {
            splitterSizes = ui->splitter->sizes();
            ui->splitter->setSizes({ 1, 0 });
        } else if (!onlyMode() && ui->newCode->hasFocus()) {
            splitterSizes = ui->splitter->sizes();
            ui->splitter->setSizes({ 0, 1 });
        }
    } else if (keyEvent->text() == "=") {
        if (!onlyMode()) {
            ui->splitter->setSizes({ 1, 1 });
        }
    } else if (keyEvent->text() == " ") {
        switchView();
    } else if (keyEvent->key() == Qt::Key_N &&
               keyEvent->modifiers() == Qt::ControlModifier) {
        if (diffList.nextEntry()) {
            ui->oldCode->clear();
            ui->newCode->clear();
            loadDiff(diffList.getCurrent());
        }
    } else if (keyEvent->key() == Qt::Key_P &&
               keyEvent->modifiers() == Qt::ControlModifier) {
        if (diffList.previousEntry()) {
            ui->oldCode->clear();
            ui->newCode->clear();
            loadDiff(diffList.getCurrent());
        }
    } else if (keyEvent->key() == Qt::Key_F1) {
        int newIndex = 1 - ui->stackedWidget->currentIndex();
        ui->stackedWidget->setCurrentIndex(newIndex);
    } else {
        return false;
    }
    return true;
}

void
ZSDiff::doSyncMatches()
{
    if (getTokenInfo(activeView()) != nullptr) {
        alignViews();
    }
}

void
ZSDiff::alignViews()
{
    syncScrolls = false;

    QTextCursor oldCursor = ui->oldCode->textCursor();
    QTextCursor newCursor = ui->newCode->textCursor();

    if (ui->oldCode->hasFocus()) {
        int leftDiff = oldCursor.block().blockNumber()
                     - ui->oldCode->firstVisibleBlock().blockNumber();
        int rightPos = newCursor.block().blockNumber() - leftDiff;
        ui->newCode->verticalScrollBar()->setSliderPosition(rightPos);
    } else {
        int rightDiff = newCursor.block().blockNumber()
                      - ui->newCode->firstVisibleBlock().blockNumber();
        int leftPos = oldCursor.block().blockNumber() - rightDiff;
        ui->oldCode->verticalScrollBar()->setSliderPosition(leftPos);
    }

    scrollDiff = ui->newCode->verticalScrollBar()->sliderPosition()
               - ui->oldCode->verticalScrollBar()->sliderPosition();

    syncScrolls = true;
}

void
ZSDiff::split(Qt::Orientation orientation)
{
    ui->splitter->setOrientation(orientation);
    activeView()->ensureCursorVisible();
    highlightMatch(activeView());
    syncScrollTo(activeView());
    if (onlyMode()) {
        ui->splitter->setSizes(splitterSizes);
    }
}

void
ZSDiff::syncScrollTo(CodeView *textEdit)
{
    if (textEdit == ui->newCode) {
        int pos = ui->newCode->verticalScrollBar()->sliderPosition();
        ui->oldCode->verticalScrollBar()->setSliderPosition(pos - scrollDiff);
    } else {
        int pos = ui->oldCode->verticalScrollBar()->sliderPosition();
        ui->newCode->verticalScrollBar()->setSliderPosition(pos + scrollDiff);
    }
}

bool
ZSDiff::onlyMode() const
{
    return ui->splitter->sizes().contains(0);
}

CodeView *
ZSDiff::activeView()
{
    return (ui->newCode->hasFocus() ? ui->newCode : ui->oldCode);
}

CodeView *
ZSDiff::inactiveView()
{
    return (ui->newCode->hasFocus() ? ui->oldCode : ui->newCode);
}

CodeView *
ZSDiff::otherView(CodeView *view)
{
    return (view == ui->newCode ? ui->oldCode : ui->newCode);
}
