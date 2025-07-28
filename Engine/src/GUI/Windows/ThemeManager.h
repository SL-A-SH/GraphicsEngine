#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QApplication>
#include <QPalette>
#include <QString>

class ThemeManager
{
public:
    enum Theme
    {
        Light,
        Dark
    };

    static void ApplyTheme(QApplication& app, Theme theme);
    static void ApplyDarkTheme(QApplication& app);
    static void ApplyLightTheme(QApplication& app);
    static QString GetDarkStyleSheet();
    static QString GetLightStyleSheet();

private:
    static QPalette CreateDarkPalette();
    static QPalette CreateLightPalette();
};

#endif // THEMEMANAGER_H 