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

    if (const QPixmap pixmap = renderDotToPixmap(dotFilePath); !pixmap.isNull()) {
        imageLabel->setPixmap(pixmap);
        imageLabel->setScaledContents(true);
    } else {
        imageLabel->setText("Failed to load graph.");
    }

    setWindowTitle(tr("Parse Tree Viewer"));
    resize(800, 600);
    setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
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
