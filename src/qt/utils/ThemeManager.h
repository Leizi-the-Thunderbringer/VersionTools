#pragma once

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

class ThemeManager
{
public:
    static void applyModernTheme(QApplication *app);
    static void applyDarkTheme(QApplication *app);
    static void applyLightTheme(QApplication *app);
    
private:
    static void setModernPalette(QPalette &palette);
    static void setDarkPalette(QPalette &palette);
    static void setLightPalette(QPalette &palette);
    static QString getModernStyleSheet();
};