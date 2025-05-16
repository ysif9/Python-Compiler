#include "symboltabledialog.hpp"

#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QApplication>
#include <QStyle>
#include <QAbstractScrollArea>
#include <QMenu>
#include <QGuiApplication>
#include <QClipboard>
#include <vector>      // For sorting map entries
#include <string>
#include <algorithm>   // For std::sort
#include <utility>     // For std::pair

// --- Helper for numeric sorting on the Index column ---
class NumericTableWidgetItem : public QTableWidgetItem {
public:
    using QTableWidgetItem::QTableWidgetItem; // Inherit constructors
    bool operator<(const QTableWidgetItem &other) const override {
        bool ok1, ok2;
        int i1 = text().toInt(&ok1), i2 = other.text().toInt(&ok2);
        if (ok1 && ok2) return i1 < i2; // Numeric comparison if possible
        return QTableWidgetItem::operator<(other); // Fallback to string comparison
    }
};
// --- End Helper ---

SymbolTableDialog::SymbolTableDialog(QWidget *parent)
    : QDialog(parent),
    tableWidget(new QTableWidget)
{
    setupUi();
    applyStyling();
    // Data is set via setSymbolData().

    // Context menu for copying cells
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableWidget, &QTableWidget::customContextMenuRequested,
            this, &SymbolTableDialog::showContextMenu);

    setWindowTitle(tr("Symbol Table"));
    resize(600, 450); // Increased default width slightly
    setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
}

SymbolTableDialog::~SymbolTableDialog() = default; // Use default destructor

void SymbolTableDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // --- Table Widget ---
    tableWidget->setColumnCount(3); // Index, Identifier, Data Type
    tableWidget->setHorizontalHeaderLabels({tr("Index"), tr("Identifier"), tr("Data Type")});
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setShowGrid(true);
    tableWidget->setWordWrap(false); // Keep types on one line if possible
    tableWidget->setSortingEnabled(false); // Disable interactive sorting
    tableWidget->setCornerButtonEnabled(false);

    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    auto *hHeader = tableWidget->horizontalHeader();
    hHeader->setHighlightSections(false);
    hHeader->setSectionsClickable(false); // Sorting is handled by setSymbolData
    hHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Index column
    hHeader->setSectionResizeMode(1, QHeaderView::Stretch);        // Identifier column
    hHeader->setSectionResizeMode(2, QHeaderView::Stretch);        // Data Type column
    hHeader->setMinimumSectionSize(80);

    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addWidget(tableWidget);

    setLayout(mainLayout);
}

void SymbolTableDialog::applyStyling()
{
    // Style remains the same - applies to the new column as well
    const QString style = R"(
        SymbolTableDialog { background-color: #2E2E2E; }
        QTableWidget {
            background-color: #1E1E1E; alternate-background-color: #252525; gridline-color: #3A3A3A;
            color: #E0E0E0; font-size: 13px; selection-background-color: #3A6EA5; selection-color: #FFFFFF;
            border: 1px solid #3A3A3A; border-radius: 4px;
        }
        QTableWidget::item { padding: 4px 6px; border-bottom: 1px solid #3A3A3A; }
        QTableWidget::item:hover { background-color: #333333; }
        QTableWidget::item:selected { background-color: #3A6EA5; color: #FFFFFF; }
        QHeaderView::section {
            background-color: #3A3A3A; color: #E0E0E0; padding: 8px 6px; border-bottom: 1px solid #3A3A3A;
            font-size: 13px; font-weight: 600;
        }
        QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; }
        QScrollBar::handle:vertical { background: #555555; min-height: 20px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #777777; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        QScrollBar:horizontal { background: transparent; height: 8px; margin: 0px; }
        QScrollBar::handle:horizontal { background: #555555; min-width: 20px; border-radius: 4px; }
        QScrollBar::handle:horizontal:hover { background: #777777; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }
    )";
    setStyleSheet(style);
}

// Updated method to populate table from the symbol map <identifier, type>
void SymbolTableDialog::setSymbolData(const std::unordered_map<std::string, std::string>& symbols)
{
    tableWidget->setRowCount(0); // Clear previous data
    tableWidget->setSortingEnabled(false); // Disable sorting during population

    // Convert map to vector of pairs for sorting by identifier name
    std::vector<std::pair<std::string, std::string>> sortedSymbols(symbols.begin(), symbols.end());
    std::sort(sortedSymbols.begin(), sortedSymbols.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first; // Sort alphabetically by identifier (key)
              });

    tableWidget->setRowCount(static_cast<int>(sortedSymbols.size()));

    for (int r = 0; r < sortedSymbols.size(); ++r) {
        const std::string& identifier = sortedSymbols[r].first;
        const std::string& dataType = sortedSymbols[r].second;

        // Index column (using row number)
        QTableWidgetItem *indexItem = new NumericTableWidgetItem(QString::number(r));
        indexItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(r, 0, indexItem);

        // Identifier column
        QTableWidgetItem *identifierItem = new QTableWidgetItem(QString::fromStdString(identifier));
        identifierItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tableWidget->setItem(r, 1, identifierItem);

        // Data Type column
        QTableWidgetItem *dataTypeItem = new QTableWidgetItem(QString::fromStdString(dataType));
        dataTypeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        // Optional: Change color based on type? e.g., unknown in red
        // if (dataType == "unknown" || dataType == "Any" || dataType == "complex_hint") {
        //     dataTypeItem->setForeground(QColor(Qt::gray)); // Example: gray for uncertain types
        // }
        tableWidget->setItem(r, 2, dataTypeItem);
    }

    // Optional: Re-enable sorting if desired, but it's disabled above
    // tableWidget->setSortingEnabled(true);
    // tableWidget->sortByColumn(0, Qt::AscendingOrder); // Sort by Index initially
}


void SymbolTableDialog::showContextMenu(const QPoint &pos)
{
    QModelIndex idx = tableWidget->indexAt(pos);
    if (!idx.isValid()) return;

    QMenu menu(this);
    QAction *copyAct = menu.addAction(tr("Copy Cell Content"));
    connect(copyAct, &QAction::triggered, this, &SymbolTableDialog::copyCell);
    menu.exec(tableWidget->viewport()->mapToGlobal(pos));
}

void SymbolTableDialog::copyCell()
{
    QModelIndex idx = tableWidget->currentIndex(); // Use current index
    if (!idx.isValid()) return;
    QString txt = tableWidget->model()->data(idx, Qt::DisplayRole).toString(); // Get display data
    QClipboard *cb = QGuiApplication::clipboard();
    cb->setText(txt);
}
