#ifndef CODEEDITOR_HPP
#define CODEEDITOR_HPP

#include <QPlainTextEdit>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class LineNumberArea;

class CodeEditor final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(const QPaintEvent *event) const;

    int lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

    void keyPressEvent(QKeyEvent *e) override; // For auto-indentation

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);

    void highlightCurrentLine();

    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
};

#endif // CODEEDITOR_HPP
