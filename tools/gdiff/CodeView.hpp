#ifndef CODEVIEW_HPP
#define CODEVIEW_HPP

#include <QPlainTextEdit>

#include <vector>

struct StablePos
{
    int line;
    int offset;

    StablePos(int line, int offset);
    explicit StablePos(QTextCursor cursor);

    QTextCursor toCursor(QTextDocument *document) const;
};

bool operator<(const StablePos &a, const StablePos &b);
bool operator==(const StablePos &a, const StablePos &b);

class CodeView : public QPlainTextEdit
{
    Q_OBJECT

    class LineColumn;

public:
    CodeView(QWidget *parent = nullptr);

    using QPlainTextEdit::firstVisibleBlock;

    void setStopPositions(std::vector<StablePos> stopPositions);
    bool goToFirstStopPosition();

signals:
    void scrolled(int pos);

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
    std::vector<StablePos> positions;
};

#endif // CODEVIEW_HPP
