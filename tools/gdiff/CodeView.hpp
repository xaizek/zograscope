#ifndef CODEVIEW_HPP
#define CODEVIEW_HPP

#include <QPlainTextEdit>

#include <vector>

class CodeView : public QPlainTextEdit
{
    class LineColumn;

public:
    CodeView(QWidget *parent = nullptr);

    using QPlainTextEdit::firstVisibleBlock;

    void setStopPositions(std::vector<int> stopPositions);
    bool goToFirstStopPosition();

private:
    void updateLineColumn(const QRect &rect, int dy);
    int computeLineColumnWidth();
    void paintLineColumn(QPaintEvent *event);

    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;

    void goDown();
    void goUp();
    void sendKey(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

private:
    LineColumn *lineColumn;
    std::vector<int> positions;
};

#endif // CODEVIEW_HPP
