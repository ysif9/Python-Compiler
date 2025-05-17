#ifndef TOKENSEQUENCEDIALOG_HPP
#define TOKENSEQUENCEDIALOG_HPP

#include <QDialog>
#include <vector>
#include "Token.hpp"


class QTableWidget;

class TokenSequenceDialog final : public QDialog {
    Q_OBJECT

public:
    explicit TokenSequenceDialog(const std::vector<Token> &tokens, QWidget *parent = nullptr);

    ~TokenSequenceDialog() override = default; // Use default destructor

private:
    void setupUi();

    void populateTable(const std::vector<Token> &tokens) const;

    static QString tokenCategoryToString(TokenCategory category);

    QTableWidget *tableWidget;
};

#endif
