#include "ThemeManager.h"
#include <QFile>
#include <QTextStream>

void ThemeManager::setTheme(QApplication &app, Theme theme)
{
    QString path;

    if (theme == Theme::Dark)
        path = ":/themes/dark.qss";   // si usas resources
    else
        path = ":/themes/light.qss";

    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        app.setStyleSheet(stream.readAll());
    }
}
