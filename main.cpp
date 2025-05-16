#include "GUI/include/mainwindow.hpp"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

void setupDarkModePalette(QApplication &app) {
    app.setStyle(QStyleFactory::create("Fusion")); // Fusion style often works well

    QPalette darkPalette;

    // Base colors
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 45));          // Window background
    darkPalette.setColor(QPalette::WindowText, Qt::white);               // General text
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));            // Editor background, list backgrounds, etc.
    darkPalette.setColor(QPalette::AlternateBase, QColor(55, 55, 55));  // Alternate row colors
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);                     // Text in editors, lists, etc.
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 60));          // Button background
    darkPalette.setColor(QPalette::ButtonText, Qt::white);               // Button text
    darkPalette.setColor(QPalette::BrightText, Qt::red);                 // Not commonly used, but set for completeness
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));          // Hyperlinks

    // Highlighting and Selection
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));     // Selected item background
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);          // Selected item text

    // Disabled states
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

    app.setPalette(darkPalette);

    // Optional: Style sheet for finer control
    app.setStyleSheet(
        "QToolTip { color: #ffffff; background-color: #2d2d2d; border: 1px solid #555555; }"
        "QMenuBar { background-color: #353535; color: #ffffff; }"
        "QMenuBar::item { background-color: #353535; color: #ffffff; }"
        "QMenuBar::item:selected { background-color: #505050; }"
        "QMenu { background-color: #353535; color: #ffffff; border: 1px solid #555555;}"
        "QMenu::item:selected { background-color: #0078d7; }" // Use highlight color
        "QStatusBar { background-color: #353535; color: #ffffff; }"
        // Add more specific styles if needed
        );
}


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    setupDarkModePalette(a); // Apply dark theme

    MainWindow w;
    w.show();
    return a.exec();
}
