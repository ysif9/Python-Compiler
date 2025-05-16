#include "TokenSequenceDialog.hpp"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QApplication>
#include <QStyle>
#include <QScrollBar> // For styling

TokenSequenceDialog::TokenSequenceDialog(const std::vector<Token>& tokens, QWidget *parent)
    : QDialog(parent),
    tableWidget(new QTableWidget(this))
{
    setupUi();
    populateTable(tokens);

    setWindowTitle(tr("Token Sequence"));
    resize(700, 500); // Adjust size as needed
    setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogListView)); // Use a relevant icon
}

QString TokenSequenceDialog::tokenCategoryToString(const TokenCategory category) {
    switch (category) {
    case TokenCategory::IDENTIFIER:   return QStringLiteral("Identifier");
    case TokenCategory::KEYWORD:      return QStringLiteral("Keyword");
    case TokenCategory::NUMBER:       return QStringLiteral("Number");
    case TokenCategory::STRING:       return QStringLiteral("String");
    case TokenCategory::PUNCTUATION:  return QStringLiteral("Punctuation");
    case TokenCategory::OPERATOR:     return QStringLiteral("Operator");
    case TokenCategory::EOFILE:       return QStringLiteral("EOF");
    case TokenCategory::UNKNOWN:      return QStringLiteral("Unknown");
    default:                          return QStringLiteral("Invalid Category");
    }
}

void TokenSequenceDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // --- Table Widget ---
    tableWidget->setColumnCount(4); // Line, Type, Lexeme, Category
    tableWidget->setHorizontalHeaderLabels({tr("Line"), tr("Type"), tr("Lexeme"), tr("Category")});
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setShowGrid(true);
    tableWidget->setWordWrap(false); // Keep lexemes on one line
    tableWidget->setSortingEnabled(false); // Keep sequence order
    tableWidget->setCornerButtonEnabled(false);

    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Header config
    auto *hHeader = tableWidget->horizontalHeader();
    hHeader->setHighlightSections(false);
    hHeader->setSectionsClickable(false); // No interactive sorting needed
    hHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Line
    hHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Type
    hHeader->setSectionResizeMode(2, QHeaderView::Stretch);         // Lexeme (give it more space)
    hHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Category
    hHeader->setMinimumSectionSize(60);

    // Auto row height
    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addWidget(tableWidget);
    setLayout(mainLayout);

    // Apply similar styling as SymbolTableDialog
    const QString style = R"(
        TokenSequenceDialog { background-color: #2E2E2E; }
        QTableWidget {
            background-color: #1E1E1E;
            alternate-background-color: #252525;
            gridline-color: #3A3A3A;
            color: #E0E0E0;
            font-size: 13px;
            selection-background-color: #3A6EA5;
            selection-color: #FFFFFF;
            border: 1px solid #3A3A3A;
            border-radius: 4px;
        }
        QTableWidget::item { padding: 4px 6px; border-bottom: 1px solid #3A3A3A; }
        QTableWidget::item:hover { background-color: #333333; }
        QTableWidget::item:selected { background-color: #3A6EA5; color: #FFFFFF; }
        QHeaderView::section {
            background-color: #3A3A3A; color: #E0E0E0; padding: 8px 6px;
            border-bottom: 1px solid #3A3A3A; font-size: 13px; font-weight: 600;
        }
        QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: #555555; min-height: 20px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #777777; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )";
    setStyleSheet(style);
}

void TokenSequenceDialog::populateTable(const std::vector<Token>& tokens) const {
    tableWidget->setRowCount(static_cast<int>(tokens.size()));

    for (int row = 0; row < tokens.size(); ++row) {
        const auto&[type, lexeme, line, category] = tokens[row];

        // Line Number
        auto *lineItem = new QTableWidgetItem(QString::number(line));
        lineItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, 0, lineItem);

        // Type (as string)
        auto *typeItem = new QTableWidgetItem(QString::fromStdString(tokenTypeToString(type)));
        typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tableWidget->setItem(row, 1, typeItem);

        // Lexeme
        auto *lexemeItem = new QTableWidgetItem(QString::fromStdString(lexeme));
        lexemeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tableWidget->setItem(row, 2, lexemeItem);

        // Category (as string)
        auto *categoryItem = new QTableWidgetItem(tokenCategoryToString(category));
        categoryItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tableWidget->setItem(row, 3, categoryItem);
    }
    // Optional: Resize rows to contents if needed, although auto-resize mode is set
    // tableWidget->resizeRowsToContents();
}
