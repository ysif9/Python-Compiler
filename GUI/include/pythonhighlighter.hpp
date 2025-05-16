#ifndef PYTHONHIGHLIGHTER_HPP
#define PYTHONHIGHLIGHTER_HPP

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector> // Include QVector

class QTextDocument;

class PythonHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    PythonHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression; // Not strictly needed for Python's single-line comments

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat; // For triple quotes
    QTextCharFormat quotationFormat;         // For single/double quotes
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat decoratorFormat;
    QTextCharFormat selfFormat;             // Highlight 'self'

    // Multi-line state tracking
    QTextCharFormat multiLineStringFormat;
    QRegularExpression tripleSingleQuoteStart;
    QRegularExpression tripleDoubleQuoteStart;
    QRegularExpression tripleSingleQuoteEnd;
    QRegularExpression tripleDoubleQuoteEnd;
};

#endif // PYTHONHIGHLIGHTER_H
