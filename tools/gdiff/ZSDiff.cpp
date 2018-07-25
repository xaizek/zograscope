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

// Parses source into a tree.
static Tree
parse(const std::string &fileName, TimeReport &tr, cpp17::pmr::monolithic *mr)
{
    std::unique_ptr<Language> lang = Language::create(fileName);

    CommonArgs args = {};
    return *buildTreeFromFile(fileName, args, tr, mr);
}

Q_DECLARE_METATYPE(TokenInfo *)

std::unique_ptr<SynHi>
ZSDiff::printTree(Tree &tree, CodeView *textEdit, bool original)
{
    QTextCursor cursor(textEdit->document());
    std::vector<int> stopPositions;

    tree.propagateStates();
    Highlighter highlighter(tree, original);
    highlighter.setPrintBrackets(false);
    highlighter.setTransparentDiffables(false);

    std::vector<ColorCane> hi = highlighter.print().splitIntoLines();

    std::vector<std::map<int, TokenInfo *>> map(hi.size());

    QString text;
    int line = 0;
    int from = 0;
    for (const ColorCane &cc : hi) {
        int lineFrom = 0;
        bool positionedOnThisLine = false;
        for (const ColorCanePiece &piece : cc) {
            if (piece.node != nullptr &&
                (piece.node->state != State::Unchanged || piece.node->moved) &&
                !positionedOnThisLine) {
                stopPositions.push_back(from);
                positionedOnThisLine = true;
            }

            text.append(QByteArray(piece.text.data(), piece.text.size()));
            const int to = from + piece.text.length();
            lineFrom += piece.text.length();
            if (piece.node != nullptr && piece.node->relative != nullptr) {
                TokenInfo *in = &info[original ? piece.node : piece.node->relative];
                map[line].emplace(lineFrom, in);

                if (original) {
                    if (in->oldFrom == 0) {
                        in->oldFrom = from;
                    }
                    in->oldTo = to;
                } else {
                    if (in->newFrom == 0) {
                        in->newFrom = from;
                    }
                    in->newTo = to;
                }
            } else {
                map[line].emplace(lineFrom, nullptr);
            }
            from = to;
        }
        ++line;
        text.append('\n');
        ++from;
    }
    if (text.size() != 0) {
        text.remove(text.size() - 1, 1);
    }
    textEdit->setPlainText(std::move(text));

    if (textEdit == ui->oldCode) {
        oldMap = std::move(map);
    } else {
        newMap = std::move(map);
    }

    textEdit->setStopPositions(std::move(stopPositions));
    textEdit->document()->setDocumentMargin(1);

    // textEdit->document()->findBlockByNumber(10).setVisible(false);

    return std::unique_ptr<SynHi>(new SynHi(textEdit->document(),
                                            std::move(hi)));
}

ZSDiff::ZSDiff(const std::string &oldFile, const std::string &newFile,
               TimeReport &tr, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ZSDiff),
      scrollDiff(0),
      syncScrolls(true),
      oldTree(&mr),
      newTree(&mr)
{
    ui->setupUi(this);

    oldTree = parse(oldFile, tr, &mr);
    newTree = parse(newFile, tr, &mr);

    compare(oldTree, newTree, tr, true, false);

    oldSynHi = printTree(oldTree, ui->oldCode, true);
    newSynHi = printTree(newTree, ui->newCode, false);

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

    connect(ui->oldCode->verticalScrollBar(), &QScrollBar::valueChanged,
            [&](int oldPos) {
        if (!syncScrolls) return;
        ui->newCode->verticalScrollBar()->setSliderPosition(oldPos +
                                                            scrollDiff);
    });
    connect(ui->newCode->verticalScrollBar(), &QScrollBar::valueChanged,
            [&](int newPos) {
        if (!syncScrolls) return;
        ui->oldCode->verticalScrollBar()->setSliderPosition(newPos -
                                                            scrollDiff);
    });

    qApp->installEventFilter(this);

    // Highlight current line on startup.
    highlightMatch(ui->oldCode);
}

void
ZSDiff::highlightMatch(QPlainTextEdit *textEdit)
{
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    lineColor = QColor(0xa0, 0xa0, 0xe0, 0x40);
    QTextCharFormat lineFormat;
    lineFormat.setBackground(lineColor);
    lineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    auto collectFormats = [&](const ColorCane &cc, QPlainTextEdit *textEdit,
                              int position,
                            QList<QTextEdit::ExtraSelection> &extraSelections) {
        int from = position;
        for (const ColorCanePiece &piece : cc) {
            const QTextCharFormat &f = cs[piece.hi];
            const int to = from + piece.text.length();
            if (f.background() != QBrush()) {
                QTextCursor cursor(textEdit->document());
                cursor.setPosition(from);
                cursor.setPosition(to, QTextCursor::KeepAnchor);
                extraSelections.append({ cursor, cs[piece.hi] });
            }
            from = to;
        }
    };

    std::vector<std::map<int, TokenInfo *>> &map = (textEdit == ui->oldCode)
                                                 ? oldMap
                                                 : newMap;
    std::map<int, TokenInfo *> &line = map[textEdit->textCursor().block().blockNumber()];
    auto it = line.upper_bound(textEdit->textCursor().positionInBlock());

    if (it == line.end() || it->second == nullptr) {
        QList<QTextEdit::ExtraSelection> newExtraSelections, oldExtraSelections;
        collectFormats(newSynHi->getHi()[ui->newCode->textCursor().blockNumber()],
                       ui->newCode,
                       ui->newCode->textCursor().block().position(),
                       newExtraSelections);
        collectFormats(oldSynHi->getHi()[ui->oldCode->textCursor().blockNumber()],
                       ui->oldCode,
                       ui->oldCode->textCursor().block().position(),
                       oldExtraSelections);
        newExtraSelections.append({ ui->newCode->textCursor(), lineFormat });
        oldExtraSelections.append({ ui->oldCode->textCursor(), lineFormat });
        ui->newCode->setExtraSelections(newExtraSelections);
        ui->oldCode->setExtraSelections(oldExtraSelections);
        return;
    }

    TokenInfo *i = it->second;

    QTextCharFormat format;
    format.setFontOverline(true);
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    syncScrolls = false;

    auto updateCursor = [&](QPlainTextEdit *textEdit, int from, int to) {
        QTextCursor sel(textEdit->document());
        sel.setPosition(from);
        sel.setPosition(to, QTextCursor::KeepAnchor);

        QTextCursor lineCursor(textEdit->document());
        lineCursor.setPosition(from);

        QList<QTextEdit::ExtraSelection> extraSelections;
        if (textEdit == ui->oldCode) {
            collectFormats(oldSynHi->getHi()[lineCursor.blockNumber()],
                           textEdit,
                           lineCursor.block().position(), extraSelections);
        } else {
            collectFormats(newSynHi->getHi()[lineCursor.blockNumber()],
                           textEdit, lineCursor.block().position(),
                           extraSelections);
        }
        extraSelections.append({ sel, format });
        extraSelections.append({ lineCursor, lineFormat });
        textEdit->setExtraSelections(extraSelections);

        QTextCursor cursor = textEdit->textCursor();
        if (cursor.position() < from || cursor.position() > to) {
            cursor.setPosition(from);
            textEdit->setTextCursor(cursor);
        }

        ui->newCode->ensureCursorVisible();

        return sel;
    };

    QTextCursor newCursor = updateCursor(ui->newCode, i->newFrom, i->newTo);
    QTextCursor oldCursor = updateCursor(ui->oldCode, i->oldFrom, i->oldTo);

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

ZSDiff::~ZSDiff()
{
    delete ui;
}

Q_DECLARE_METATYPE(QTextDocumentFragment)

void
ZSDiff::switchView()
{
    if (ui->newCode->hasFocus()) {
        ui->oldCode->setFocus();
    } else {
        ui->newCode->setFocus();
    }
}

bool
ZSDiff::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) {
        return QObject::eventFilter(obj, event);
    }

    auto onlyMode = [this]() {
        return ui->splitter->sizes().contains(0);
    };

    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->text() == "q") {
        close();
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
