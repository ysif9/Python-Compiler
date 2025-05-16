// mainwindow.cpp
#include "MainWindow.hpp"
#include "CodeEditor.hpp"
#include "PythonHighlighter.hpp"
#include "FindReplaceDialog.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "SymbolTableDialog.hpp"
#include "TokenSequenceDialog.hpp"


#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QSettings>
#include <QLabel>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>

#include "Parser.hpp"
#include "ParserTreeDialog.hpp"

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      // textEdit member removed
      findDialog(nullptr),
      isUntitled(true),
      lexer_instance(nullptr),
      // Initialize menu pointers
      fileMenu(nullptr),
      editMenu(nullptr),
      searchMenu(nullptr),
      lexerMenu(nullptr),
      parserMenu(nullptr),
      helpMenu(nullptr),
      // Initialize file action pointers
      newAct(nullptr),
      openAct(nullptr),
      saveAct(nullptr),
      saveAsAct(nullptr),
      exitAct(nullptr),
      // Initialize edit action pointers
      undoAct(nullptr),
      redoAct(nullptr),
      cutAct(nullptr),
      copyAct(nullptr),
      pasteAct(nullptr),
      // Initialize search action pointers
      findAct(nullptr),
      replaceAct(nullptr),
      // Initialize help action pointers
      runAct(nullptr),
      viewSymbolTableAct(nullptr),
      // Initialize other action pointers (already present)
      viewTokenSequenceAct(nullptr),
      aboutAct(nullptr),
      aboutQtAct(nullptr) {
    editor = new CodeEditor(this);
    setCentralWidget(editor);

    // Apply Syntax Highlighting
    highlighter = new PythonHighlighter(editor->document());

    createActions();
    createMenus();
    createStatusBar();

    readSettings(); // Load window state

    // Connect signals from editor
    connect(editor->document(), &QTextDocument::contentsChanged,
            this, &MainWindow::documentWasModified);
    connect(editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatusBar);
    connect(editor->document(), &QTextDocument::undoAvailable,
            this, &MainWindow::updateUndoRedoActions);
    connect(editor->document(), &QTextDocument::redoAvailable,
            this, &MainWindow::updateUndoRedoActions);

    // Connect contentsChanged to update lexer action state
    connect(editor->document(), &QTextDocument::contentsChanged,
            this, &MainWindow::updateLexerActionsState);

    setCurrentFile(QString()); // Initialize window title etc.
    setUnifiedTitleAndToolBarOnMac(true);

    updateUndoRedoActions(); // Set initial undo/redo state
    updateLexerActionsState(); // Set initial lexer action state
    disableLexerResultActions(); // Ensure result actions start disabled
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        writeSettings(); // Save window state
        event->accept();
    } else {
        event->ignore();
    }
}

// --- File Actions ---
void MainWindow::newFile() {
    if (maybeSave()) {
        isUntitled = true;
        editor->clear();
        setCurrentFile(QString());
        editor->document()->setModified(false);
        setWindowModified(false);
        updateUndoRedoActions();
        updateLexerActionsState(); // Update run button state (will be disabled)
        disableLexerResultActions(); // Disable result actions for new file
    }
}

void MainWindow::open() {
    if (maybeSave()) {
        const QString fileName = QFileDialog::getOpenFileName(this,
                                                              tr("Open Python File"), "",
                                                              tr("Python Files (*.py *.pyw);;All Files (*)"));
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

bool MainWindow::save() {
    if (isUntitled || currentFile.isEmpty()) {
        return saveAs();
    }
    return saveFile(currentFile);
}

bool MainWindow::saveAs() {
    QString selectedFilterStr;
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Python File"),
                                                    currentFile.isEmpty() ? "." : currentFile,
                                                    tr("Python Files (*.py *.pyw);;All Files (*)"),
                                                    &selectedFilterStr);
    if (fileName.isEmpty())
        return false;

    if (QFileInfo(fileName).suffix().isEmpty()) {
        if (selectedFilterStr == tr("Python Files (*.py *.pyw)")) {
            fileName += ".py";
        }
    }

    return saveFile(fileName);
}

// --- Edit Actions ---
void MainWindow::undo() const { editor->undo(); }
void MainWindow::redo() const { editor->redo(); }
void MainWindow::cut() const { editor->cut(); }
void MainWindow::copy() const { editor->copy(); }
void MainWindow::paste() const { editor->paste(); }

// --- Search/Replace Actions ---
void MainWindow::find() {
    if (!findDialog) {
        findDialog = new FindReplaceDialog(this);
        connect(findDialog, &FindReplaceDialog::findNext, this, &MainWindow::findNext);
        connect(findDialog, &FindReplaceDialog::findPrevious, this, &MainWindow::findPrevious);
        connect(findDialog, &FindReplaceDialog::replaceNext, this, &MainWindow::replaceNext);
        connect(findDialog, &FindReplaceDialog::replaceAll, this, &MainWindow::replaceAll);
    }
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void MainWindow::replace() {
    find();
}

void MainWindow::findNext(const QString &str, const QTextDocument::FindFlags flags) {
    if (str.isEmpty()) return;

    if (!editor->find(str, flags)) {
        QTextCursor cursor = editor->textCursor();
        const bool searchingBackward = flags.testFlag(QTextDocument::FindBackward);
        const QMessageBox::StandardButton wrapReply = QMessageBox::question(this, tr("Wrap Search"),
                                                                            tr(
                                                                                "Search string '%1' not found.\nWrap search from %2?")
                                                                            .arg(str).arg(
                                                                                searchingBackward
                                                                                    ? tr("end")
                                                                                    : tr("beginning")),
                                                                            QMessageBox::Yes | QMessageBox::No);
        if (wrapReply == QMessageBox::Yes) {
            if (searchingBackward)
                cursor.movePosition(QTextCursor::End);
            else
                cursor.movePosition(QTextCursor::Start);
            editor->setTextCursor(cursor);
            if (!editor->find(str, flags)) {
                QMessageBox::information(this, tr("Not Found"), tr("The search string '%1' was not found.").arg(str));
            }
        }
    }
}

void MainWindow::findPrevious(const QString &str, const QTextDocument::FindFlags flags) {
    findNext(str, flags | QTextDocument::FindBackward);
}

void MainWindow::replaceNext(const QString &findStr, const QString &replaceStr, QTextDocument::FindFlags flags,
                             const bool replaceAllMode) {
    if (findStr.isEmpty()) return;

    QTextCursor cursor = editor->textCursor();
    bool found = false;

    bool selectionMatches = false;
    if (cursor.hasSelection()) {
        selectionMatches = (flags & QTextDocument::FindCaseSensitively)
                               ? cursor.selectedText().compare(findStr, Qt::CaseSensitive) == 0
                               : cursor.selectedText().compare(findStr, Qt::CaseInsensitive) == 0;
    }

    if (selectionMatches) {
        cursor.insertText(replaceStr);
        found = true;
    } else {
        if (editor->find(findStr, flags)) {
            editor->textCursor().insertText(replaceStr);
            found = true;
        } else {
            if (!replaceAllMode) {
                QMessageBox::information(this, tr("Not Found"),
                                         tr("The search string '%1' was not found.").arg(findStr));
            }
            return;
        }
    }

    if (found && !replaceAllMode) {
        findNext(findStr, flags);
    }
}


void MainWindow::replaceAll(const QString &findStr, const QString &replaceStr, const QTextDocument::FindFlags flags) {
    if (findStr.isEmpty()) return;

    int count = 0;
    const QTextCursor originalCursor = editor->textCursor();
    editor->moveCursor(QTextCursor::Start);

    const QTextDocument::FindFlags searchFlags = flags & ~QTextDocument::FindBackward;

    editor->document()->blockSignals(true);

    while (editor->find(findStr, searchFlags)) {
        if (QTextCursor cursor = editor->textCursor(); cursor.hasSelection()) {
            cursor.insertText(replaceStr);
            count++;
        } else {
            break;
        }
    }
    editor->document()->blockSignals(false);

    updateUndoRedoActions();
    if (editor->textCursor().position() == originalCursor.position()) {
        updateStatusBar();
    }

    if (count > 0) {
        QMessageBox::information(this, tr("Replace All"), tr("%n occurrence(s) replaced.", "", count));
    } else {
        QMessageBox::information(this, tr("Replace All"), tr("The search string '%1' was not found.").arg(findStr));
    }
}

// --- Lexer Actions ---

void MainWindow::runLexer() {
    const QString currentCode = editor->toPlainText();
    if (currentCode.isEmpty()) {
        statusBar()->showMessage(tr("Nothing to analyze."), 3000);
        disableLexerResultActions();
        return;
    }

    statusBar()->showMessage(tr("Running lexer..."));
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    disableLexerResultActions(); // Clears lastTokens/lastSymbols and disables buttons

    bool lexerSuccess = true;
    std::vector<Lexer_error> lexerErrors; // Store errors here

    try {
        const string codeStdString = currentCode.toStdString();
        lexer_instance = std::make_unique<Lexer>(codeStdString);
        Lexer &lexer = *lexer_instance;

        // --- Phase 1: Tokenization ---
        Token token;
        lastTokens.clear();
        do {
            token = lexer.nextToken();
            if (token.type != TokenType::TK_EOF) {
                lastTokens.push_back(token);
            }
            if (token.type == TokenType::TK_UNKNOWN && token.lexeme != "") {
                // It's often better to rely on the lexer's internal error reporting
                // qWarning() << "Lexer encountered unknown token:" << QString::fromStdString(token.lexeme)
                //            << "at line" << token.line;
                // lexerSuccess = false; // Let the lexer decide if it's fatal via its error list
            }
        } while (token.type != TokenType::TK_EOF);


        // --- Phase 2: Process Types and Populate Symbol Table ---
        lexer.processIdentifierTypes();


        // --- Phase 3: Retrieve Results AND Errors ---
        lastSymbols = lexer.getSymbolTable();
        lexerErrors = lexer.getErrors(); // <-- Get the errors from the lexer

        // Check if any errors were reported by the lexer
        if (!lexerErrors.empty()) {
            lexerSuccess = false; // Mark as not fully successful if errors occurred
        }


        // --- Phase 4: Update UI based on success and results ---
        // Enable parser
        if (lexerSuccess) {
            viewSymbolTableAct->setEnabled(!lastSymbols.empty());
            viewTokenSequenceAct->setEnabled(!lastTokens.empty());

            const int symbolCount = static_cast<int>(lastSymbols.size());
            statusBar()->showMessage(
                tr("Lexer finished successfully. %n token(s), %1 symbol(s) found.", "", lastTokens.size()).
                arg(symbolCount), 5000);
        } else {
            // Keep view actions disabled (already handled by disableLexerResultActions)
            statusBar()->showMessage(tr("Lexer finished with %n error(s).", "", lexerErrors.size()), 5000);
        }
    } catch (const std::exception &e) {
        lexerSuccess = false;
        QGuiApplication::restoreOverrideCursor(); // Restore cursor before showing message box
        QMessageBox::critical(this, tr("Lexer Runtime Error"),
                              tr("A runtime error occurred during lexical analysis:\n%1").arg(e.what()));
        statusBar()->showMessage(tr("Lexer failed."), 3000);
        // disableLexerResultActions(); // Already called at the start
    } catch (...) {
        lexerSuccess = false;
        QGuiApplication::restoreOverrideCursor(); // Restore cursor before showing message box
        QMessageBox::critical(this, tr("Lexer Runtime Error"),
                              tr("An unknown runtime error occurred during lexical analysis."));
        statusBar()->showMessage(tr("Lexer failed."), 3000);
        // disableLexerResultActions(); // Already called at the start
    }

    // Ensure cursor is always restored
    if (QGuiApplication::overrideCursor()) {
        QGuiApplication::restoreOverrideCursor();
    }

    // --- Show Error Dialog AFTER lexing is complete ---
    if (!lexerErrors.empty()) {
        ErrorDialog errorDialog(lexerErrors, this); // Create the dialog
        errorDialog.exec(); // Show it modally
    }

    // --- Final UI State Update ---
    // Ensure view actions are appropriately enabled/disabled based on final state
    viewSymbolTableAct->setEnabled(lexerSuccess && !lastSymbols.empty());
    viewTokenSequenceAct->setEnabled(lexerSuccess && !lastTokens.empty());
    parseAct->setEnabled(lexerSuccess && !lastSymbols.empty());
}

void MainWindow::showSymbolTable() {
    // Action should be disabled if no symbols, but double-check
    if (!viewSymbolTableAct->isEnabled() || lastSymbols.empty()) {
        QMessageBox::information(this, tr("Symbol Table"), tr("No symbols found or lexer not run successfully yet."));
        return;
    }

    const auto dialog = new SymbolTableDialog(this);
    dialog->setSymbolData(lastSymbols);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::showTokenSequence() {
    // Action should be disabled if no tokens, but double-check
    if (!viewTokenSequenceAct->isEnabled() || lastTokens.empty()) {
        QMessageBox::information(this, tr("Token Sequence"), tr("No tokens found or lexer not run successfully yet."));
        return;
    }

    const auto dialog = new TokenSequenceDialog(lastTokens, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// --- Other Slots & Helpers ---
void MainWindow::documentWasModified() {
    setWindowModified(editor->document()->isModified());
    updateUndoRedoActions();
    // Lexer state update handled by direct connection now
}

void MainWindow::updateStatusBar() const {
    const QTextCursor cursor = editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int col = cursor.positionInBlock() + 1;
    statusBar()->showMessage(tr("Line: %1, Col: %2").arg(line).arg(col));
}

void MainWindow::updateUndoRedoActions() const {
    undoAct->setEnabled(editor->document()->isUndoAvailable());
    redoAct->setEnabled(editor->document()->isRedoAvailable());
}

void MainWindow::updateLexerActionsState() {
    const bool hasText = !editor->toPlainText().isEmpty();
    runAct->setEnabled(hasText);

    // If text is modified or empty, invalidate previous lexer results
    if (editor->document()->isModified() || !hasText) {
        disableLexerResultActions();
    }
    // View actions are only enabled inside runLexer() on success.
}

void MainWindow::disableLexerResultActions() {
    // Check if actions exist before disabling
    if (viewSymbolTableAct) viewSymbolTableAct->setEnabled(false);
    if (viewTokenSequenceAct) viewTokenSequenceAct->setEnabled(false);
    if (parseAct) parseAct->setEnabled(false);
    // Clear stored results
    lastTokens.clear();
    lastSymbols.clear();
}

// --- Parser Actions ---
void MainWindow::runParser() {
    if (lexer_instance == nullptr) {
        statusBar()->showMessage(tr("Lexer not initialized."), 3000);
        viewParserTreeAct->setEnabled(false); // Clears lastTokens/lastSymbols and disables buttons
        return;
    }

    statusBar()->showMessage(tr("Running parser..."));
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    viewParserTreeAct->setEnabled(false); // Clears lastTokens/lastSymbols and disables buttons

    bool parser_success_flag = true;
    std::vector<std::string> parser_errors;

    try {
        Parser parser_instance(*lexer_instance);

        // --- Handle parser logic later ----
        parser_instance.parse();
        parser_errors = parser_instance.getErrors();
        dotFilePath = parser_instance.getDotFilePath();

        // Check if the parser reported any errors
        if (!parser_errors.empty()) {
            parser_success_flag = false; // Mark as not fully successful if errors occurred
        }

        if (parser_success_flag) {
            // TODO: Add logic to check the dot file exists or ast tree successful
            viewParserTreeAct->setEnabled(true);

            // TODO: Display success message and stats
            // const int symbolCount = static_cast<int>(lastSymbols.size());
            statusBar()->showMessage(
                tr("Parser finished successfully.")
            );
            // arg(symbolCount), 5000);
        } else {
            statusBar()->showMessage(tr("Parser finished with %n error(s).", "", parser_errors.size()), 5000);
        }
    } catch (const std::exception &e) {
        parser_success_flag = false;
        QGuiApplication::restoreOverrideCursor(); // Restore cursor before showing message box
        QMessageBox::critical(this, tr("Parser Runtime Error"),
                              tr("A runtime error occurred during syntax analysis:\n%1").arg(e.what()));
        statusBar()->showMessage(tr("Parser failed."), 3000);
    } catch (...) {
        parser_success_flag = false;
        QGuiApplication::restoreOverrideCursor(); // Restore cursor before showing message box
        QMessageBox::critical(this, tr("Parser Runtime Error"),
                              tr("An unknown runtime error occurred during syntax analysis."));
        statusBar()->showMessage(tr("Parser failed."), 3000);
    }

    // Ensure cursor is always restored
    if (QGuiApplication::overrideCursor()) {
        QGuiApplication::restoreOverrideCursor();
    }

    if (!parser_errors.empty()) {
        ErrorDialog errorDialog(parser_errors, this); // Create the dialog
        errorDialog.exec(); // Show it modally
    }

    // --- Final UI State Update ---
    //TODO: Ensure view actions are appropriately enabled/disabled based on final state
}

// TODO: Make sure this is enabled and disabled correctly
void MainWindow::showParserTree() {
    if (!viewParserTreeAct->isEnabled()) {
        QMessageBox::information(this, tr("Parser Tree"),
                                 tr("No parser tree found or parser not run successfully yet."));
        return;
    }
    if (!dotFilePath.empty()) {
        const auto dialog = new ParserTreeDialog(dotFilePath.data(), this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    } else {
        QMessageBox::information(this, tr("Parser Tree"),
                                 tr("No parser tree found or parser not run successfully yet."));
    }
}


// --- UI Creation ---
void MainWindow::createActions() {
    // File Actions
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // Edit Actions
    undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo the last operation"));
    connect(undoAct, &QAction::triggered, this, &MainWindow::undo);

    redoAct = new QAction(tr("&Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo the last undone operation"));
    connect(redoAct, &QAction::triggered, this, &MainWindow::redo);

    cutAct = new QAction(tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);

    copyAct = new QAction(tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);

    pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);

    // Search Actions
    findAct = new QAction(tr("&Find..."), this);
    findAct->setShortcuts(QKeySequence::Find);
    findAct->setStatusTip(tr("Find text in the document"));
    connect(findAct, &QAction::triggered, this, &MainWindow::find);

    replaceAct = new QAction(tr("&Replace..."), this);
    replaceAct->setShortcuts(QKeySequence::Replace);
    replaceAct->setStatusTip(tr("Replace text in the document"));
    connect(replaceAct, &QAction::triggered, this, &MainWindow::replace);

    // Lexer Actions
    runAct = new QAction(tr("&Run Lexer"), this);
    runAct->setStatusTip(tr("Run lexical analysis on the current code"));
    // runAct->setShortcut(Qt::Key_F5); // Optional shortcut
    connect(runAct, &QAction::triggered, this, &MainWindow::runLexer);
    runAct->setEnabled(false); // Start disabled

    viewSymbolTableAct = new QAction(tr("Show &Symbol Table"), this);
    viewSymbolTableAct->setStatusTip(tr("View the symbol table from the last lexer run"));
    connect(viewSymbolTableAct, &QAction::triggered, this, &MainWindow::showSymbolTable);
    viewSymbolTableAct->setEnabled(false); // Start disabled

    viewTokenSequenceAct = new QAction(tr("Show &Token Sequence"), this);
    viewTokenSequenceAct->setStatusTip(tr("View the token sequence from the last lexer run"));
    connect(viewTokenSequenceAct, &QAction::triggered, this, &MainWindow::showTokenSequence);
    viewTokenSequenceAct->setEnabled(false); // Start disabled

    // Parser Action
    parseAct = new QAction(tr("&Run Parser"), this);
    parseAct->setStatusTip(tr("Run syntax parsing on the current code"));
    connect(parseAct, &QAction::triggered, this, &MainWindow::runParser);
    parseAct->setEnabled(false); // Start disabled

    viewParserTreeAct = new QAction(tr("Show &Parser Tree"), this);
    viewParserTreeAct->setStatusTip(tr("View the parser tree from the last parser run"));
    connect(viewParserTreeAct, &QAction::triggered, this, &MainWindow::showParserTree);
    viewParserTreeAct->setEnabled(false); // Start disabled

    // Help Actions
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    // connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
    aboutAct->setEnabled(false);

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    // Connect editor signals for enabling/disabling edit actions
    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    connect(editor, &QPlainTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
    connect(editor, &QPlainTextEdit::copyAvailable, copyAct, &QAction::setEnabled);

    // Undo/Redo are handled by updateUndoRedoActions()
    undoAct->setEnabled(false);
    redoAct->setEnabled(false);
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    searchMenu = menuBar()->addMenu(tr("&Search"));
    searchMenu->addAction(findAct);
    searchMenu->addAction(replaceAct);

    lexerMenu = menuBar()->addMenu(tr("&Lexer"));
    lexerMenu->addAction(runAct);
    lexerMenu->addSeparator();
    lexerMenu->addAction(viewSymbolTableAct);
    lexerMenu->addAction(viewTokenSequenceAct);

    // TODO: Create parser actions
    parserMenu = menuBar()->addMenu(tr("&Parser"));
    parserMenu->addAction(parseAct);
    parserMenu->addSeparator();
    parserMenu->addAction(viewParserTreeAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createStatusBar() const {
    statusBar()->showMessage(tr("Ready"));
}

// --- Settings & File Handling ---
void MainWindow::readSettings() {
    const QSettings settings;
    if (const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray(); geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() / 2, availableGeometry.height() * 2 / 3);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings() const {
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
}

bool MainWindow::maybeSave() {
    if (!editor->document()->isModified())
        return true;
    const QMessageBox::StandardButton ret
            = QMessageBox::warning(this, tr("Py2Cpp"),
                                   tr("The document has been modified.\n"
                                       "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
        case QMessageBox::Save:
            return save();
        case QMessageBox::Cancel:
            return false;
        case QMessageBox::Discard:
            return true;
        default:
            break;
    }
    return true; // Should be unreachable
}

void MainWindow::loadFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Py2Cpp"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QTextStream in(&file);
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    editor->document()->blockSignals(true);
    editor->setPlainText(in.readAll());
    editor->document()->blockSignals(false);

#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded: %1").arg(strippedName(fileName)), 2000);
    isUntitled = false;
    editor->document()->setModified(false);
    setWindowModified(false);
    editor->moveCursor(QTextCursor::Start);
    updateUndoRedoActions();
    updateLexerActionsState(); // Update run button state
    disableLexerResultActions(); // Disable results after loading new file
}

bool MainWindow::saveFile(const QString &fileName) {
    QString errorMessage;

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    QSaveFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        // Consider setting codec, e.g., out.setCodec("UTF-8");
        out << editor->toPlainText();
        if (!file.commit()) {
            errorMessage = tr("Cannot write file %1:\n%2.")
                    .arg(QDir::toNativeSeparators(fileName), file.errorString());
        }
    } else {
        errorMessage = tr("Cannot open file %1 for writing:\n%2.")
                .arg(QDir::toNativeSeparators(fileName), file.errorString());
    }
    QGuiApplication::restoreOverrideCursor();

    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Py2Cpp"), errorMessage);
        return false;
    }

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved: %1").arg(strippedName(fileName)), 2000);
    isUntitled = false;
    editor->document()->setModified(false);
    setWindowModified(false);

    return true;
}

void MainWindow::setCurrentFile(const QString &fileName) {
    currentFile = fileName;
    QString shownName = tr("Untitled");
    if (!currentFile.isEmpty()) {
        shownName = strippedName(currentFile);
        isUntitled = false;
    } else {
        isUntitled = true;
    }
    setWindowTitle(tr("%1[*] - %2").arg(shownName, tr("Py2Cpp")));
    setWindowModified(editor->document()->isModified());
}

QString MainWindow::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName();
}
