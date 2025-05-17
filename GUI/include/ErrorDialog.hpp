#ifndef ERRORDIALOG_HPP
#define ERRORDIALOG_HPP

#include <QDialog>
#include <vector>
#include "lexer.hpp"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

class ErrorDialog final : public QDialog {
    Q_OBJECT

public:
    // Constructor: Takes the list of errors and the parent widget
    explicit ErrorDialog(const std::vector<Lexer_error> &errors, QWidget *parent = nullptr);
    explicit ErrorDialog(const std::vector<std::string> &errors, QWidget *parent = nullptr);

    void setupUi();

    ~ErrorDialog() override = default; // Use default destructor

private:
    // UI Elements
    QPlainTextEdit *errorDisplay;
    QDialogButtonBox *buttonBox;

    // Helper function to format and display errors
    void displayErrors(const std::vector<Lexer_error> &errors) const;
    void displayErrors(const std::vector<std::string> &errors) const;
};

#endif // ERRORDIALOG_HPP
