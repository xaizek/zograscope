#ifndef ZSDIFF_HPP
#define ZSDIFF_HPP

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QMainWindow>

#include "pmr/monolithic.hpp"
#include "tree.hpp"

#include "CodeView.hpp"
#include "GuiColorScheme.hpp"

namespace Ui {
class ZSDiff;
}

class QPlainTextEdit;
class QTextCharFormat;

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

class ZSDiff : public QMainWindow
{
    Q_OBJECT

    struct SideInfo;

public:
    ZSDiff(const std::string &oldFile, const std::string &newFile,
           TimeReport &tr, QWidget *parent = nullptr);
    ~ZSDiff();

private:
    SideInfo printTree(Tree &tree, CodeView *textEdit, bool original);
    void highlightMatch(QPlainTextEdit *textEdit);
    TokenInfo * getTokenInfo(QPlainTextEdit *textEdit);

    void switchView();

    virtual bool eventFilter(QObject *obj, QEvent *event) override;

    void alignViews();
    void syncScrollTo(CodeView *textEdit);
    bool onlyMode() const;

private:
    Ui::ZSDiff *ui;
    std::unordered_map<const Node *, TokenInfo> info;
    int scrollDiff;
    bool syncScrolls;
    cpp17::pmr::monolithic mr;
    Tree oldTree;
    Tree newTree;
    std::unique_ptr<SynHi> oldSynHi;
    std::unique_ptr<SynHi> newSynHi;
    GuiColorScheme cs;
    std::vector<std::map<int, TokenInfo *>> oldMap, newMap;
    QList<int> splitterSizes;
};

#endif // ZSDIFF_HPP
