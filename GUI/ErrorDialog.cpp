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

    // Create the text display area
    errorDisplay = new QPlainTextEdit(this);
    errorDisplay->setReadOnly(true); // User cannot edit the errors
    errorDisplay->setLineWrapMode(QPlainTextEdit::NoWrap); // Prevent line wrapping initially

    // Create standard buttons (just Close for this dialog)
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);

    // Connect the Close button signal to the dialog's reject slot (which closes it)
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Create the main layout
    // TODO: Apparently leaked memory here
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(errorDisplay); // Add text area
    mainLayout->addWidget(buttonBox); // Add button box

    // Set the layout for the dialog
    setLayout(mainLayout);

    // Populate the display with the provided errors
    displayErrors(errors);
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
