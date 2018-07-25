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

#include "GuiColorScheme.hpp"

namespace Ui {
class ZSDiff;
}

class QPlainTextEdit;
class QTextCharFormat;

class CodeView;

class Node;
class TimeReport;
class SynHi;

struct TokenInfo
{
    int oldFrom = 0;
    int oldTo = 0;
    int newFrom = 0;
    int newTo = 0;
};

class ZSDiff : public QMainWindow
{
    Q_OBJECT

public:
    ZSDiff(const std::string &oldFile, const std::string &newFile,
           TimeReport &tr, QWidget *parent = nullptr);
    ~ZSDiff();

private:
    std::unique_ptr<SynHi> printTree(Tree &tree, CodeView *textEdit,
                                     bool original);
    void highlightMatch(QPlainTextEdit *textEdit);

    void switchLayout();
    void switchView();

    virtual bool eventFilter(QObject *obj, QEvent *event) override;

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
};

#endif // ZSDIFF_HPP
