#ifndef LINENUMBERAREA_HPP
#define LINENUMBERAREA_HPP

#include "CodeEditor.hpp"

class LineNumberArea final : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

#endif // LINENUMBERAREA_HPP
