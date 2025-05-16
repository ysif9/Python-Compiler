#include "findreplacedialog.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox> // Use group box for options

FindReplaceDialog::FindReplaceDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Find and Replace"));
    setMinimumWidth(350); // Adjust as needed

    // --- Create UI Elements ---
    findLineEdit = new QLineEdit(this);
    replaceLineEdit = new QLineEdit(this);

    findNextButton = new QPushButton(tr("&Find Next"), this);
    replaceButton = new QPushButton(tr("&Replace"), this);
    replaceAllButton = new QPushButton(tr("Replace &All"), this);
    closeButton = new QPushButton(tr("Close"), this);

    caseCheckBox = new QCheckBox(tr("Match &case"), this);
    wholeWordCheckBox = new QCheckBox(tr("Match &whole word"), this);
    backwardCheckBox = new QCheckBox(tr("Search &backward"), this);

    // --- Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel(tr("Find what:"), this));
    findLayout->addWidget(findLineEdit);
    mainLayout->addLayout(findLayout);

    QHBoxLayout *replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel(tr("Replace with:"), this));
    replaceLayout->addWidget(replaceLineEdit);
    mainLayout->addLayout(replaceLayout);

    // Options Group
    QGroupBox *optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    optionsLayout->addWidget(caseCheckBox);
    optionsLayout->addWidget(wholeWordCheckBox);
    optionsLayout->addWidget(backwardCheckBox);
    mainLayout->addWidget(optionsGroup);

    // Button Layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // Push buttons to the right
    buttonLayout->addWidget(findNextButton);
    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(replaceAllButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    // --- Connections ---
    connect(findNextButton, &QPushButton::clicked, this, &FindReplaceDialog::on_findNextButton_clicked);
    connect(replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::on_replaceButton_clicked);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::on_replaceAllButton_clicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject); // Or accept(), depending on desired behavior

    // Enable/disable buttons based on input
    connect(findLineEdit, &QLineEdit::textChanged, this, &FindReplaceDialog::updateButtonStates);
    connect(replaceLineEdit, &QLineEdit::textChanged, this, &FindReplaceDialog::updateButtonStates); // Optional

    updateButtonStates(); // Initial state

    // Focus find input on open
    findLineEdit->setFocus();
}

FindReplaceDialog::~FindReplaceDialog()
{
    // No Ui:: object to delete if done manually
}

QTextDocument::FindFlags FindReplaceDialog::getFindFlags()
{
    QTextDocument::FindFlags flags;
    if (caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (wholeWordCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;
    if (backwardCheckBox->isChecked())
        flags |= QTextDocument::FindBackward;
    return flags;
}


void FindReplaceDialog::on_findNextButton_clicked()
{
    QString findText = findLineEdit->text();
    if (!findText.isEmpty()) {
        if(backwardCheckBox->isChecked()) {
            emit findPrevious(findText, getFindFlags());
        } else {
            emit findNext(findText, getFindFlags());
        }
    }
}


void FindReplaceDialog::on_replaceButton_clicked()
{
    QString findText = findLineEdit->text();
    QString replaceText = replaceLineEdit->text();
    if (!findText.isEmpty()) {
        emit replaceNext(findText, replaceText, getFindFlags());
    }
}

void FindReplaceDialog::on_replaceAllButton_clicked()
{
    QString findText = findLineEdit->text();
    QString replaceText = replaceLineEdit->text();
    if (!findText.isEmpty()) {
        emit replaceAll(findText, replaceText, getFindFlags());
    }
}

void FindReplaceDialog::updateButtonStates()
{
    bool hasFindText = !findLineEdit->text().isEmpty();
    findNextButton->setEnabled(hasFindText);
    replaceButton->setEnabled(hasFindText);
    replaceAllButton->setEnabled(hasFindText);
}
