#include "GUI/include/MainWindow.hpp"
#include <QStyleFactory>

#include "ThemeUtility.hpp"


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    ThemeUtility::setupDarkModePalette(a);; // Apply dark theme

    MainWindow w;
    w.show();
    return a.exec();
}
