#include "ZSDiff.hpp"

#include <QList>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextStream>
#include <QScrollBar>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include "dtl/dtl.hpp"
#include "pmr/monolithic.hpp"

#include "tooling/common.hpp"
#include "utils/optional.hpp"
#include "utils/time.hpp"
#include "ColorCane.hpp"
#include "Highlighter.hpp"
#include "TreeBuilder.hpp"
#include "Language.hpp"
#include "STree.hpp"
#include "colors.hpp"
#include "compare.hpp"
#include "tree.hpp"

#include "CodeView.hpp"
#include "SynHi.hpp"
#include "ui_zsdiff.h"

const int matchProperty = QTextFormat::UserProperty + 2;

struct ZSDiff::SideInfo
{
    std::vector<ColorCane> hi;
    std::vector<std::string> lines;
    std::vector<std::map<int, TokenInfo *>> map;
};

// Parses source into a tree.
static Tree
parse(const std::string &fileName, TimeReport &tr, cpp17::pmr::monolithic *mr)
{
    std::unique_ptr<Language> lang = Language::create(fileName);

    CommonArgs args = {};
    return *buildTreeFromFile(fileName, args, tr, mr);
}

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

    std::vector<std::string> lines;
    int line = 0;
    for (const ColorCane &cc : hi) {
        std::string text;

        int lineFrom = 0;
        bool positionedOnThisLine = false;
        for (const ColorCanePiece &piece : cc) {
            if (piece.node != nullptr &&
                (piece.node->state != State::Unchanged || piece.node->moved) &&
                !positionedOnThisLine) {
                stopPositions.emplace_back(line, lineFrom);
                positionedOnThisLine = true;
            }

            text += piece.text;
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
        lines.emplace_back(std::move(text));
    }

    textEdit->setStopPositions(std::move(stopPositions));
    textEdit->document()->setDocumentMargin(1);

    // textEdit->document()->findBlockByNumber(10).setVisible(false);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                      Qt::TextSelectableByKeyboard);

    return { std::move(hi), std::move(lines), std::move(map) };
}

ZSDiff::ZSDiff(const std::string &oldFile, const std::string &newFile,
               TimeReport &tr, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ZSDiff),
      scrollDiff(0),
      syncScrolls(true),
      syncMatches(false),
      oldTree(&mr),
      newTree(&mr)
{
    ui->setupUi(this);

    oldTree = parse(oldFile, tr, &mr);
    newTree = parse(newFile, tr, &mr);

    compare(oldTree, newTree, tr, true, false);

    QTextDocument *oldDoc = ui->oldCode->document();
    QTextDocument *newDoc = ui->newCode->document();

    SideInfo leftSide = printTree(oldTree, ui->oldCode, true);
    oldSynHi.reset(new SynHi(oldDoc, std::move(leftSide.hi)));
    oldMap = std::move(leftSide.map);
    SideInfo rightSide = printTree(newTree, ui->newCode, false);
    newSynHi.reset(new SynHi(newDoc, std::move(rightSide.hi)));
    newMap = std::move(rightSide.map);

    auto cmp = [](const std::string &a, const std::string &b) {
        // XXX: need dice comparison here too?  
        return (a == b);
    };

    oldDoc->documentLayout()->registerHandler(blankLineAttr.getType(),
                                              &blankLineAttr);
    newDoc->documentLayout()->registerHandler(blankLineAttr.getType(),
                                              &blankLineAttr);

    QTextCharFormat blankLineFormat;
    blankLineFormat.setObjectType(blankLineAttr.getType());
    auto addBlank = [&blankLineFormat](CodeView *view) {
        QTextCursor c = view->textCursor();
        c.insertText(QString(QChar::ObjectReplacementCharacter),
                     blankLineFormat);
    };

    dtl::Diff<std::string, std::vector<std::string>,
              decltype(cmp)> diff(leftSide.lines, rightSide.lines, cmp);
    diff.compose();
    int leftLine = 0, rightLine = 0;
    int leftState = -1, rightState = -1;
    for (const auto &x : diff.getSes().getSequence()) {
        switch (x.second.type) {
            std::string *left, *right;

            case dtl::SES_DELETE:
                left = &leftSide.lines[x.second.beforeIdx - 1];
                ui->oldCode->insertPlainText(QByteArray(left->data(),
                                                        left->size()));
                oldDoc->lastBlock().setUserState(leftLine++);
                addBlank(ui->newCode);
                break;
            case dtl::SES_ADD:
                right = &rightSide.lines[x.second.afterIdx - 1];
                ui->newCode->insertPlainText(QByteArray(right->data(),
                                                        right->size()));
                newDoc->lastBlock().setUserState(rightLine++);
                addBlank(ui->oldCode);
                break;
            case dtl::SES_COMMON:
                left = &leftSide.lines[x.second.beforeIdx - 1];
                right = &rightSide.lines[x.second.afterIdx - 1];
                ui->oldCode->insertPlainText(QByteArray(left->data(),
                                                        left->size()));
                oldDoc->lastBlock().setUserState(leftLine++);
                ui->newCode->insertPlainText(QByteArray(right->data(),
                                                        right->size()));
                newDoc->lastBlock().setUserState(rightLine++);
                break;
        }
        leftState = oldDoc->lastBlock().userState();
        rightState = newDoc->lastBlock().userState();
        ui->oldCode->insertPlainText("\n");
        ui->newCode->insertPlainText("\n");
    }

    // Remove extra line.
    oldDoc->lastBlock().setUserState(leftState);
    ui->oldCode->textCursor().deletePreviousChar();
    newDoc->lastBlock().setUserState(rightState);
    ui->newCode->textCursor().deletePreviousChar();

    ui->newCode->moveCursor(QTextCursor::Start);
    ui->oldCode->moveCursor(QTextCursor::Start);

    auto onPosChanged = [&](QPlainTextEdit *textEdit) {
        QTextCursor cursor = textEdit->textCursor();
        if (cursor.hasSelection()) return;
        highlightMatch(textEdit);
    };
    connect(ui->oldCode, &QPlainTextEdit::cursorPositionChanged,
            [=]() { onPosChanged(ui->oldCode); });
    connect(ui->newCode, &QPlainTextEdit::cursorPositionChanged,
            [=]() { onPosChanged(ui->newCode); });

    connect(ui->oldCode, &CodeView::scrolled, [&](int /*pos*/) {
        if (syncScrolls) {
            syncScrollTo(ui->oldCode);
        }
    });
    connect(ui->newCode, &CodeView::scrolled, [&](int /*pos*/) {
        if (syncScrolls) {
            syncScrollTo(ui->newCode);
        }
    });

    qApp->installEventFilter(this);

    // Navigate to first change in old or new version of the code and  highlight
    // current line.
    if (ui->oldCode->goToFirstStopPosition()) {
        highlightMatch(ui->oldCode);
    } else {
        ui->newCode->goToFirstStopPosition();
        highlightMatch(ui->newCode);
        ui->newCode->setFocus();
    }
}

void
ZSDiff::highlightMatch(QPlainTextEdit *textEdit)
{
    struct CursInfo {
        QTextCursor cursor;
        bool needUpdate;
    };

    QTextCharFormat lineFormat;
    lineFormat.setBackground(QColor(0xa0, 0xa0, 0xe0, 0x40));
    lineFormat.setProperty(QTextFormat::FullWidthSelection, true);

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

    TokenInfo *i = getTokenInfo(textEdit);
    if (i == nullptr) {
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

        newExtraSelections.append({ ui->newCode->textCursor(), lineFormat });
        oldExtraSelections.append({ ui->oldCode->textCursor(), lineFormat });
        ui->newCode->setExtraSelections(newExtraSelections);
        ui->oldCode->setExtraSelections(oldExtraSelections);
        return;
    }

    QTextCharFormat format;
    format.setFontOverline(true);
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    syncScrolls = false;

    auto updateCursor = [&](QPlainTextEdit *textEdit,
                            StablePos fromPos, StablePos toPos) {
        int from = fromPos.offset;
        int to = toPos.offset;
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

        QTextCursor sel(textEdit->document());
        sel.setPosition(from);
        sel.setPosition(to, QTextCursor::KeepAnchor);

        QTextCursor lineCursor(textEdit->document());
        lineCursor.setPosition(from);

        QList<QTextEdit::ExtraSelection> extraSelections;
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
        extraSelections.append({ lineCursor, lineFormat });
        textEdit->setExtraSelections(extraSelections);

        QTextCursor cursor = textEdit->textCursor();
        int currentPos = cursor.position();
        cursor.setPosition(from);
        return CursInfo { cursor, (currentPos < from || currentPos > to) };
    };

    CursInfo newInfo = updateCursor(ui->newCode, i->newFrom, i->newTo);
    CursInfo oldInfo = updateCursor(ui->oldCode, i->oldFrom, i->oldTo);

    syncScrolls = true;

    if (oldInfo.needUpdate) {
        ui->oldCode->setTextCursor(oldInfo.cursor);
    } else {
        ui->oldCode->setTextCursor(ui->oldCode->textCursor());
    }
    if (newInfo.needUpdate) {
        ui->newCode->setTextCursor(newInfo.cursor);
    } else {
        ui->newCode->setTextCursor(ui->newCode->textCursor());
    }

    if (syncMatches) {
        alignViews();
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
    } else if (keyEvent->text() == "s") {
        ui->splitter->setOrientation(Qt::Vertical);
        if (onlyMode()) {
            ui->splitter->setSizes(splitterSizes);
        }
    } else if (keyEvent->text() == "v") {
        ui->splitter->setOrientation(Qt::Horizontal);
        if (onlyMode()) {
            ui->splitter->setSizes(splitterSizes);
        }
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
