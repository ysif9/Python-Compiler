// mainwindow.hpp
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTextDocument> // For FindFlags
#include <vector>        // For storing tokens
#include <string>        // For storing symbols
#include "ErrorDialog.hpp"


struct Token;

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}

class QAction;
class QMenu;
class QPlainTextEdit;
class CodeEditor;
class PythonHighlighter;
class FindReplaceDialog;
class SymbolTableDialog;
class TokenSequenceDialog;
QT_END_NAMESPACE

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File Actions
    void newFile();

    void open();

    bool save();

    bool saveAs();

    // Edit Actions
    void undo() const;

    void redo() const;

    void cut() const;

    void copy() const;

    void paste() const;

    // Search Actions
    void find();

    void findNext(const QString &str, QTextDocument::FindFlags flags);

    void findPrevious(const QString &str, QTextDocument::FindFlags flags);

    void replace(); // Slot to show the dialog
    void replaceNext(const QString &findStr, const QString &replaceStr, QTextDocument::FindFlags flags,
                     bool replaceAllMode = false);

    void replaceAll(const QString &findStr, const QString &replaceStr, QTextDocument::FindFlags flags);

    // *** Lexer Actions ***
    void runLexer();

    void showSymbolTable();

    void showTokenSequence();

    // Other Slots
    void documentWasModified();

    void updateStatusBar() const;

    void updateUndoRedoActions() const;

    void updateLexerActionsState();

private:
    void createActions();

    void createMenus();

    void createStatusBar() const;

    void readSettings();

    void writeSettings() const;

    bool maybeSave();

    void loadFile(const QString &fileName);

    bool saveFile(const QString &fileName);

    void setCurrentFile(const QString &fileName);

    static QString strippedName(const QString &fullFileName);

    void disableLexerResultActions();

    void runParser();

    void showParserTree();

    CodeEditor *editor;
    PythonHighlighter *highlighter;
    FindReplaceDialog *findDialog;

    QString currentFile;
    bool isUntitled;

    // *** Lexer Results ***
    std::vector<Token> lastTokens;
    std::unordered_map<std::string, std::string> lastSymbols;

    // Menus
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *searchMenu;
    QMenu *lexerMenu;
    QMenu *parserMenu;
    QMenu *helpMenu;

    // Actions
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *undoAct;
    QAction *redoAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *findAct;
    QAction *replaceAct;
    // *** Lexer Actions ***
    QAction *runAct;
    QAction *viewSymbolTableAct;
    QAction *viewTokenSequenceAct; // Action for the token sequence view
    // *** Parser Actions ***
    QAction *parseAct;
    QAction *viewParserTreeAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
};
#endif // MAINWINDOW_HPP
