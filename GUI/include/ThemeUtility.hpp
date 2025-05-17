#ifndef THEMEUTILITY_HPP
#define THEMEUTILITY_HPP

#include <QApplication>

class ThemeUtility {
public:
    static void setupDarkModePalette(QApplication &app);

private:
    ThemeUtility() = delete; // Prevent instantiation
};

#endif //THEMEUTILITY_HPP
