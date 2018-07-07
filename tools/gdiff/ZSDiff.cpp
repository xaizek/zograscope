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
#include "GuiColorScheme.hpp"
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

void
ZSDiff::printTree(Tree &tree, CodeView *textEdit, bool original)
{
    QTextCursor cursor(textEdit->document());
    GuiColorScheme cs;
    std::vector<int> stopPositions;

    tree.propagateStates();
    Highlighter highlighter(tree, original);
    highlighter.setPrintBrackets(false);
    highlighter.setTransparentDiffables(false);

    int lastLinePos = 0;
    for (const ColorCanePiece &piece : highlighter.print()) {
        const int from = cursor.position();
        const int linePos = from - cursor.positionInBlock();

        if (piece.node != nullptr && piece.node->state != State::Unchanged) {
            if (stopPositions.empty() || lastLinePos != linePos) {
                stopPositions.push_back(from);
                lastLinePos = linePos;
            }
        }

        if (piece.node == nullptr || piece.node->relative == nullptr) {
            cursor.insertText(piece.text.c_str(), cs[piece.hi]);
            continue;
        }

        TokenInfo *in = &info[original ? piece.node : piece.node->relative];

        QVariant v;
        v.setValue(in);

        QTextCharFormat format = cs[piece.hi];
        format.setProperty(matchProperty, v);

        cursor.insertText(piece.text.c_str(), format);
        const int to = cursor.position();

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
    }

    textEdit->setStopPositions(std::move(stopPositions));
}

ZSDiff::ZSDiff(const std::string &oldFile, const std::string &newFile,
               TimeReport &tr, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ZSDiff),
      scrollDiff(0),
      syncScrolls(true)
{
    ui->setupUi(this);

    cpp17::pmr::monolithic mr;

    Tree oldTree = parse(oldFile, tr, &mr);
    Tree newTree = parse(newFile, tr, &mr);

    compare(oldTree, newTree, tr, true, false);

    printTree(oldTree, ui->oldCode, true);
    printTree(newTree, ui->newCode, false);

    ui->newCode->moveCursor(QTextCursor::Start);
    ui->oldCode->moveCursor(QTextCursor::Start);

    connect(ui->mainToolBar->addAction("switch layout"), &QAction::triggered,
            this, &ZSDiff::switchLayout);

    auto onPosChanged = [&](QPlainTextEdit *textEdit) {
        QTextCursor cursor = textEdit->textCursor();
        if (cursor.hasSelection()) return;
        if (!cursor.charFormat().hasProperty(matchProperty) &&
            !cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::Right);
        }
        highlightMatch(cursor.charFormat());
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
}

void
ZSDiff::highlightMatch(const QTextCharFormat &f)
{
    if (!f.hasProperty(matchProperty)) {
        ui->newCode->setExtraSelections({});
        ui->oldCode->setExtraSelections({});
        return;
    }

    auto i = f.property(matchProperty).value<TokenInfo *>();

    QTextCursor oldCursor(ui->oldCode->document());
    oldCursor.setPosition(i->oldFrom);
    oldCursor.setPosition(i->oldTo, QTextCursor::KeepAnchor);

    QTextCursor newCursor(ui->newCode->document());
    newCursor.setPosition(i->newFrom);
    newCursor.setPosition(i->newTo, QTextCursor::KeepAnchor);

    QTextCharFormat format;
    format.setFontOverline(true);
    format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    syncScrolls = false;

    auto updateCursor = [](QPlainTextEdit *textEdit, int from, int to) {
        QTextCursor cursor = textEdit->textCursor();
        if (cursor.position() < from || cursor.position() > to) {
            cursor.setPosition(from);
            textEdit->setTextCursor(cursor);
        }
    };

    ui->newCode->setExtraSelections({ { newCursor, format } });
    updateCursor(ui->newCode, i->newFrom, i->newTo);
    ui->newCode->ensureCursorVisible();

    ui->oldCode->setExtraSelections({ { oldCursor, format } });
    updateCursor(ui->oldCode, i->oldFrom, i->oldTo);
    ui->oldCode->ensureCursorVisible();

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
ZSDiff::switchLayout()
{
    if (ui->splitter->orientation() == Qt::Vertical) {
        ui->splitter->setOrientation(Qt::Horizontal);
    } else {
        ui->splitter->setOrientation(Qt::Vertical);
    }
}

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

    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->text() == "q") {
        close();
    } else if (keyEvent->text() == "s") {
        switchLayout();
    } else if (keyEvent->text() == " ") {
        switchView();
    } else {
        return false;
    }
    return true;
}
