#include "ErrorDialog.hpp"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QString>
#include <QTextCursor> // Include for QTextCursor

// Constructor implementation
ErrorDialog::ErrorDialog(const std::vector<Lexer_error> &errors, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Lexer Errors"));
    setMinimumSize(500, 300); // Set a reasonable minimum size
    setupUi();
    displayErrors(errors);
}

ErrorDialog::ErrorDialog(const std::vector<string> &errors, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Parser Errors"));
    setMinimumSize(500, 300); // Set a reasonable minimum size
    setupUi();
    displayErrors(errors);
}

void ErrorDialog::setupUi() {
    errorDisplay = new QPlainTextEdit(this);
    errorDisplay->setReadOnly(true);
    errorDisplay->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font("monospace");
    font.setStyleHint(QFont::TypeWriter);
    errorDisplay->setFont(font);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(errorDisplay);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}


// Implementation of the displayErrors helper function
void ErrorDialog::displayErrors(const std::vector<Lexer_error> &errors) const {
    errorDisplay->clear(); // Clear previous content

    if (errors.empty()) {
        errorDisplay->setPlainText(tr("No errors reported by the lexer."));
        return;
    }

    // Set a monospace font for better alignment if needed
    QFont font("monospace");
    font.setStyleHint(QFont::TypeWriter);
    errorDisplay->setFont(font);

    for (const auto &[message, line, lexeme]: errors) {
        // Format the error message nicely
        QString errorLine = tr("Line %1: %2")
                .arg(line)
                .arg(QString::fromStdString(message));

        // Include the problematic lexeme if it's not empty
        if (!lexeme.empty()) {
            errorLine += tr(" (near '%1')").arg(QString::fromStdString(lexeme));
        }

        errorDisplay->appendPlainText(errorLine);
    }

    // Move cursor to the beginning of the text
    errorDisplay->moveCursor(QTextCursor::Start);
}

void ErrorDialog::displayErrors(const std::vector<std::string> &errors) const {
    errorDisplay->clear();

    if (errors.empty()) {
        errorDisplay->setPlainText(tr("No errors reported by the Parser."));
        return;
    }

    for (const auto &errorMsg : errors) {
        errorDisplay->appendPlainText(QString::fromStdString(errorMsg));
    }

    errorDisplay->moveCursor(QTextCursor::Start);
}
