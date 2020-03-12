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

#ifndef ZOGRASCOPE__TOOLS__GDIFF__ZSDIFF_HPP__
#define ZOGRASCOPE__TOOLS__GDIFF__ZSDIFF_HPP__

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QMainWindow>

#include "pmr/monolithic.hpp"
#include "tree.hpp"

#include "BlankLineAttr.hpp"
#include "CodeView.hpp"
#include "DiffList.hpp"
#include "FoldTextAttr.hpp"
#include "GuiColorScheme.hpp"

namespace Ui {
class ZSDiff;
}

class QPlainTextEdit;

class Node;
class TimeReport;
class SynHi;

struct TokenInfo
{
    StablePos oldFrom = { 0, 0 };
    StablePos oldTo = { 0, 0 };
    StablePos newFrom = { 0, 0 };
    StablePos newTo = { 0, 0 };
};

enum class LaunchMode
{
    Standalone,
    GitExt,
    Staged,
    Unstaged,
    Commit,
};

class ZSDiff : public QMainWindow
{
    Q_OBJECT

    struct SideInfo;
    struct CodeBuffer;

public:
    ZSDiff(LaunchMode launchMode, DiffList diffList, TimeReport &tr,
           QWidget *parent = nullptr);
    ~ZSDiff();

private:
    void loadDiff(const DiffEntry &diffEntry);
    void updateTitle();
    SideInfo printTree(Tree &tree, CodeView *textEdit, bool original);
    void diffAndPrint(TimeReport &tr);
    void highlightMatch(QPlainTextEdit *textEdit);
    void syncOtherCursor(QPlainTextEdit *textEdit);
    void resolveRange(QPlainTextEdit *textEdit,
                      StablePos fromPos, StablePos toPos,
                      int &from, int &to);
    TokenInfo * getTokenInfo(QPlainTextEdit *textEdit);
    void fold();
    void unfold();
    void updateView(CodeView *view);
    void updateLayout();

    void switchView();

    virtual bool eventFilter(QObject *obj, QEvent *event) override;

    void doSyncMatches();
    // Aligns cursor in the other pane to make it match position of a token
    // that corresponds to one under the cursor of the current pane.
    void alignViews();
    void split(Qt::Orientation orientation);
    void syncScrollTo(CodeView *textEdit);
    bool onlyMode() const;
    CodeView * activeView();
    CodeView * inactiveView();
    CodeView * otherView(CodeView *view);

private:
    Ui::ZSDiff *ui;
    std::unordered_map<const Node *, TokenInfo> info;
    // Whether diff entry is already loaded.
    bool loaded;
    int scrollDiff;
    bool syncScrolls;
    bool syncMatches;
    bool firstTimeFocus;
    bool folded;
    cpp17::pmr::monolithic mr;
    Tree oldTree;
    Tree newTree;
    std::unique_ptr<SynHi> oldSynHi;
    std::unique_ptr<SynHi> newSynHi;
    GuiColorScheme cs;
    std::vector<std::map<int, TokenInfo *>> oldMap, newMap;
    QList<int> splitterSizes;
    BlankLineAttr blankLineAttr;
    FoldTextAttr foldTextAttr;
    // Whether respective lines supposed to be folded.
    std::vector<bool> leftFolded, rightFolded;
    TimeReport &timeReport;
    LaunchMode launchMode;
    DiffList diffList;
};

#endif // ZOGRASCOPE__TOOLS__GDIFF__ZSDIFF_HPP__
