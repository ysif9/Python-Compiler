#include "GUI/include/MainWindow.hpp"
#include <QStyleFactory>

#include "ThemeUtility.hpp"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ThemeUtility::setupDarkModePalette(app);

    MainWindow window;
    window.show();
    return app.exec();
}
