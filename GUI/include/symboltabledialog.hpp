#ifndef SYMBOLTABLEDIALOG_HPP
#define SYMBOLTABLEDIALOG_HPP

#include <QDialog>
#include <unordered_map> // Changed from unordered_set
#include <string>

class QTableWidget;

class SymbolTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolTableDialog(QWidget *parent = nullptr);
    ~SymbolTableDialog() override; // Mark override

    // Method to set the data from the lexer's processed symbol table
    // Accepts a map of <identifier, type_string>
    void setSymbolData(const std::unordered_map<std::string, std::string>& symbols);

private slots:
    void showContextMenu(const QPoint &pos);
    void copyCell();

private:
    void setupUi();
    void applyStyling();

    QTableWidget *tableWidget;
};

#endif // SYMBOLTABLEDIALOG_HPP
