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
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    lineColor = QColor(0xa0, 0xa0, 0xe0, 0x40);
    QTextCharFormat lineFormat;
    lineFormat.setBackground(lineColor);
    lineFormat.setProperty(QTextFormat::FullWidthSelection, true);

    auto collectFormats = [](const QTextBlock &block, QPlainTextEdit *textEdit,
                            QList<QTextEdit::ExtraSelection> &extraSelections) {
        for (auto it = block.begin(), end = block.end(); it != end; ++it) {
            const QTextFragment &tf = it.fragment();
            if (tf.isValid() && tf.charFormat().background() != QBrush()) {
                QTextCursor cursor(textEdit->document());
                cursor.setPosition(tf.position());
                cursor.setPosition(tf.position() + tf.length(),
                                   QTextCursor::KeepAnchor);
                extraSelections.append({ cursor, tf.charFormat() });
            }
        }
    };

    if (!f.hasProperty(matchProperty)) {
        QList<QTextEdit::ExtraSelection> newExtraSelections, oldExtraSelections;
        collectFormats(ui->newCode->textCursor().block(), ui->newCode,
                       newExtraSelections);
        collectFormats(ui->oldCode->textCursor().block(), ui->oldCode,
                       oldExtraSelections);
        newExtraSelections.append({ ui->newCode->textCursor(), lineFormat });
        oldExtraSelections.append({ ui->oldCode->textCursor(), lineFormat });
        ui->newCode->setExtraSelections(newExtraSelections);
        ui->oldCode->setExtraSelections(oldExtraSelections);
        return;
    }

    auto i = f.property(matchProperty).value<TokenInfo *>();

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
        collectFormats(lineCursor.block(), textEdit, extraSelections);
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
