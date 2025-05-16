#include "ParserTreeDialog.hpp"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QProcess>
#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QStyle>

ParserTreeDialog::ParserTreeDialog(const QString &dotFilePath, QWidget *parent)
    : QDialog(parent),
      imageLabel(new QLabel(this)) {
    setupUi();
    const QPixmap pixmap = renderDotToPixmap(dotFilePath);
    if (!pixmap.isNull()) {
        imageLabel->setPixmap(pixmap);
        imageLabel->setScaledContents(true);
    } else {
        imageLabel->setText("Failed to load graph.");
    }

    setWindowTitle(tr("Parse Tree Viewer"));
    if (!pixmap.isNull()) {
        const QSize imageSize = pixmap.size();
        constexpr auto maxSize = QSize(2000, 2000); // Optional: set a max size to avoid huge dialogs

        // Add some padding to ensure full image fits inside the dialog
        constexpr int paddingWidth = 40;  // Account for borders/scrollbar
        constexpr int paddingHeight = 80; // Account for title bar/layout

        const QSize paddedSize = imageSize + QSize(paddingWidth, paddingHeight);

        // Constrain to max size
        const QSize finalSize = paddedSize.boundedTo(maxSize);

        resize(finalSize);
    } else {
        resize(600, 800); // fallback
    }
    setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_DialogYesButton));
}

void ParserTreeDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(imageLabel);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);

    const QString style = R"(
        ParserTreeDialog { background-color: #2E2E2E; }
        QLabel {
            background-color: #1E1E1E;
            border: 1px solid #3A3A3A;
        }
        QScrollArea {
            background-color: #2E2E2E;
            border: none;
        }
    )";
    setStyleSheet(style);
}

QPixmap ParserTreeDialog::renderDotToPixmap(const QString &dotFilePath) {
    qDebug() << "Trying to load DOT file:" << dotFilePath;
    if (!QFile::exists(dotFilePath)) {
        qWarning() << "DOT file not found:" << dotFilePath;
        return {};
    }
    qDebug() << "file exists";

    // Create temporary output PNG file
    const QString imagePath = QDir::tempPath() + "/parse_tree_output.png";

    // Run Graphviz to convert DOT -> PNG
    QProcess dotProcess;
    const QStringList args = {"-Tpng", dotFilePath, "-o", imagePath};
    dotProcess.start("dot", args);
    if (!dotProcess.waitForFinished()) {
        qWarning() << "Graphviz process failed:" << dotProcess.errorString();
        return {};
    }

    QPixmap pixmap;
    pixmap.load(imagePath);
    return pixmap;
}
