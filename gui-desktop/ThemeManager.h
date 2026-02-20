#pragma once

#include <QApplication>

enum class Theme {
    Dark,
    Light
};

class ThemeManager {
public:
    static void setTheme(QApplication &app, Theme theme);
};
