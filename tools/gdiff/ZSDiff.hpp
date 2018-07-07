#ifndef ZSDIFF_HPP
#define ZSDIFF_HPP

#include <string>
#include <unordered_map>

#include <QMainWindow>

namespace Ui {
class ZSDiff;
}

class QPlainTextEdit;
class QTextCharFormat;

class CodeView;

class Node;
class Tree;
class TimeReport;

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
    void printTree(Tree &tree, CodeView *textEdit, bool original);
    void highlightMatch(const QTextCharFormat &f);

    void switchLayout();
    void switchView();

    virtual bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::ZSDiff *ui;
    std::unordered_map<const Node *, TokenInfo> info;
    int scrollDiff;
    bool syncScrolls;
};

#endif // ZSDIFF_HPP
