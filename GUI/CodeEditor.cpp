#include "include/CodeEditor.hpp"
#include "LineNumberArea.hpp"
#include <QPainter>
#include <QTextBlock>
#include <QRegularExpression>
#include <QKeyEvent> // Include for QKeyEvent

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);

    // Connect signals for line number area updates
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Set font - choose a good monospaced font
    QFont font;
    font.setFamily("Consolas"); // Or Courier New, Monaco, Menlo, etc.
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPointSize(11); // Adjust size as needed
    setFont(font);

    // Tab stop width (Python standard is 4 spaces)
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    // Optionally disable line wrap for code
    // setLineWrapMode(QPlainTextEdit::NoWrap);
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // Add some padding
    const int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, const int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;


        constexpr auto lineColor = QColor(50, 50, 60);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(const QPaintEvent *event) const {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(35, 35, 35)); // Line number background color

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    // Colors for line numbers
    QColor currentLineNumberColor = QColor(200, 200, 200); // Brighter for current line
    QColor otherLineNumberColor = QColor(120, 120, 120); // Dimmer for other lines
    int currentBlockNumber = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(blockNumber == currentBlockNumber ? currentLineNumberColor : otherLineNumberColor);
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        const QTextCursor cursor = textCursor();
        const QTextBlock currentBlock = cursor.block();
        QString indent = "";

        // Get previous block's indentation
        if (currentBlock.isValid()) {
            const QString prevText = currentBlock.text();
            const QRegularExpression re("^\\s*"); // Match leading whitespace
            if (const QRegularExpressionMatch match = re.match(prevText); match.hasMatch()) {
                indent = match.captured(0);
            }

            // Check if the current line ends with a colon (increase indent)
            if (prevText.trimmed().endsWith(':')) {
                indent += QString(tabStopDistance() / fontMetrics().horizontalAdvance(' '), ' '); // Add one tab level
            }
        }

        // Let QPlainTextEdit handle the newline insertion first
        QPlainTextEdit::keyPressEvent(e);

        // Now insert the calculated indentation
        textCursor().insertText(indent);
    } else if (e->key() == Qt::Key_Tab && e->modifiers() == Qt::NoModifier) {
        // Handle Tab key for indentation/completion (basic example)
        // In a real IDE, this would check context for auto-completion triggers
        if (const QTextCursor cursor = textCursor(); cursor.hasSelection()) {
            // Indent selected block (implementation omitted for brevity,
            // would involve iterating selected blocks and adding spaces/tabs)
            // QPlainTextEdit::keyPressEvent(e); // Or custom logic
            QPlainTextEdit::keyPressEvent(e); // Fallback for now
        } else {
            // Insert tab spaces
            textCursor().insertText(QString(tabStopDistance() / fontMetrics().horizontalAdvance(' '), ' '));
        }
    } else if (e->key() == Qt::Key_Backtab) {
        // Shift+Tab for un-indent
        // Handle Shift+Tab for un-indentation (implementation omitted for brevity)
        // Would involve removing spaces/tabs from the start of the line/selection
        QPlainTextEdit::keyPressEvent(e); // Fallback for now
    } else {
        // Let the base class handle other keys
        QPlainTextEdit::keyPressEvent(e);
    }
}
