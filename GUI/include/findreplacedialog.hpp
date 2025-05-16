#ifndef FINDREPLACEDIALOG_HPP
#define FINDREPLACEDIALOG_HPP

#include <QDialog>
#include <QPlainTextEdit> // For FindFlags

namespace Ui {
class FindReplaceDialog;
}

class QLineEdit;
class QPushButton;
class QCheckBox;

class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget *parent = nullptr);
    ~FindReplaceDialog();

signals:
    void findNext(const QString &str, QTextDocument::FindFlags flags);
    void findPrevious(const QString &str, QTextDocument::FindFlags flags);
    void replaceNext(const QString &findStr, const QString &replaceStr, QTextDocument::FindFlags flags, bool replaceAllMode = false);
    void replaceAll(const QString &findStr, const QString &replaceStr, QTextDocument::FindFlags flags);


private slots:
    void on_findNextButton_clicked();
    void on_replaceButton_clicked();
    void on_replaceAllButton_clicked();
    void updateButtonStates();

private:
    Ui::FindReplaceDialog *ui; // Using UI form is easier here, but doing manually for example

    // Manual UI elements if not using .ui file
    QLineEdit *findLineEdit;
    QLineEdit *replaceLineEdit;
    QPushButton *findNextButton;
    QPushButton *findPreviousButton; // Added
    QPushButton *replaceButton;
    QPushButton *replaceAllButton;
    QPushButton *closeButton;
    QCheckBox *caseCheckBox;
    QCheckBox *wholeWordCheckBox;
    QCheckBox *backwardCheckBox; // Replaces findPreviousButton logic

    QTextDocument::FindFlags getFindFlags();
};

#endif // FINDREPLACEDIALOG_HPP
