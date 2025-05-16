#include "pythonhighlighter.hpp"
#include <QColor>

PythonHighlighter::PythonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // --- Define Formats (Elegant Dark Mode Contrast) ---
    // Keyword: Light blue, bold
    keywordFormat.setForeground(QColor(86, 156, 214)); // VS Code blue
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns = {
        QStringLiteral("\\bimport\\b"), QStringLiteral("\\bfrom\\b"), QStringLiteral("\\bas\\b"),
        QStringLiteral("\\bclass\\b"), QStringLiteral("\\bdef\\b"),
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belif\\b"), QStringLiteral("\\belse\\b"),
        QStringLiteral("\\bfor\\b"), QStringLiteral("\\bin\\b"), QStringLiteral("\\bwhile\\b"),
        QStringLiteral("\\btry\\b"), QStringLiteral("\\bexcept\\b"), QStringLiteral("\\bfinally\\b"),
        QStringLiteral("\\bwith\\b"), QStringLiteral("\\bpass\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\byield\\b"),
        QStringLiteral("\\blambda\\b"),
        QStringLiteral("\\bTrue\\b"), QStringLiteral("\\bFalse\\b"), QStringLiteral("\\bNone\\b"),
        QStringLiteral("\\band\\b"), QStringLiteral("\\bor\\b"), QStringLiteral("\\bnot\\b"),
        QStringLiteral("\\bis\\b"),
        QStringLiteral("\\bassert\\b"), QStringLiteral("\\bdel\\b"), QStringLiteral("\\bglobal\\b"),
        QStringLiteral("\\bnonlocal\\b"), QStringLiteral("\\braise\\b"),
        QStringLiteral("\\bawait\\b"), QStringLiteral("\\basync\\b") // Python 3.5+
    };
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Self: Italic, slightly different color
    selfFormat.setForeground(QColor(180, 180, 180)); // Light grey/off-white
    selfFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("\\bself\\b"));
    rule.format = selfFormat;
    highlightingRules.append(rule);

    // Class name: Light green/teal
    classFormat.setForeground(QColor(78, 201, 176)); // VS Code green-ish
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("\\bclass\\s+([A-Za-z_][A-Za-z0-9_]*)\\b"));
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Function name: Yellow/Gold
    functionFormat.setForeground(QColor(220, 220, 170)); // VS Code yellow-ish
    rule.pattern = QRegularExpression(QStringLiteral("\\bdef\\s+([A-Za-z_][A-Za-z0-9_]*)\\b"));
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Decorators: Purple/Magenta
    decoratorFormat.setForeground(QColor(190, 120, 220));
    rule.pattern = QRegularExpression(QStringLiteral("@[A-Za-z_][A-Za-z0-9_.]*"));
    rule.format = decoratorFormat;
    highlightingRules.append(rule);

    // Numbers: Orange/Peach
    numberFormat.setForeground(QColor(181, 206, 168)); // VS Code number color
    rule.pattern = QRegularExpression(QStringLiteral("\\b[0-9]+\\.?[0-9]*([eE][-+]?[0-9]+)?\\b")); // Integers, floats, scientific
    rule.format = numberFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression(QStringLiteral("\\b0[xX][0-9a-fA-F]+\\b")); // Hex
    rule.format = numberFormat;
    highlightingRules.append(rule);


    // --- Strings ---
    // Single and double quotes: Green/Brownish
    quotationFormat.setForeground(QColor(206, 145, 120)); // VS Code string color
    rule.pattern = QRegularExpression(QStringLiteral("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"")); // Double quotes
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression(QStringLiteral("'[^'\\\\]*(\\\\.[^'\\\\]*)*'")); // Single quotes
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // --- Comments ---
    // Single-line comments: Grey, Italic
    singleLineCommentFormat.setForeground(QColor(110, 110, 110)); // Dark grey
    singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral("#[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // --- Multi-line Strings / Docstrings (Triple Quotes) ---
    multiLineStringFormat.setForeground(QColor(110, 145, 120)); // Different shade for multi-line
    tripleSingleQuoteStart = QRegularExpression(QStringLiteral("'''"));
    tripleDoubleQuoteStart = QRegularExpression(QStringLiteral("\"\"\""));
    // End patterns need lookbehind to avoid matching start patterns as end patterns immediately
    tripleSingleQuoteEnd = QRegularExpression(QStringLiteral("(?<!')'''"));
    tripleDoubleQuoteEnd = QRegularExpression(QStringLiteral("(?<!\")\"\"\""));
}


void PythonHighlighter::highlightBlock(const QString &text) {
    // 1. Apply standard highlighting rules
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            // Handle potential overlaps for class/function names after keywords
            if (rule.format == classFormat || rule.format == functionFormat) {
                setFormat(match.capturedStart(1), match.capturedLength(1), rule.format);
            } else {
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
    setCurrentBlockState(0); // Default state is normal code

    // 2. Handle multi-line strings (Docstrings)
    int startIndex = 0;
    int currentState = previousBlockState(); // Get state from previous block

    // Determine start state if not continuing a multi-line string
    if (currentState != 1 && currentState != 2) {
        startIndex = text.indexOf(tripleDoubleQuoteStart);
        if (startIndex == -1) {
            startIndex = text.indexOf(tripleSingleQuoteStart);
            if (startIndex != -1) currentState = 2; // Start triple single quote
        } else {
            currentState = 1; // Start triple double quote
        }
    } else {
        startIndex = 0; // Already inside a multi-line string
    }


    while (startIndex >= 0) {
        QRegularExpression endExpression = (currentState == 1) ? tripleDoubleQuoteEnd : tripleSingleQuoteEnd;
        QRegularExpressionMatch endMatch = endExpression.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) { // Multi-line string continues to next block
            setCurrentBlockState(currentState);
            commentLength = text.length() - startIndex;
        } else { // Multi-line string ends in this block
            commentLength = endIndex - startIndex + endMatch.capturedLength();
            setCurrentBlockState(0); // Return to normal state
        }

        setFormat(startIndex, commentLength, multiLineStringFormat);

        // Find the next potential start after the current one ends
        int nextDouble = text.indexOf(tripleDoubleQuoteStart, startIndex + commentLength);
        int nextSingle = text.indexOf(tripleSingleQuoteStart, startIndex + commentLength);

        if (nextDouble == -1 && nextSingle == -1) {
            startIndex = -1; // No more starts in this block
        } else if (nextDouble != -1 && (nextSingle == -1 || nextDouble < nextSingle)) {
            startIndex = nextDouble;
            currentState = 1;
        } else {
            startIndex = nextSingle;
            currentState = 2;
        }
    }
}
