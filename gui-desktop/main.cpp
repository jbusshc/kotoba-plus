#include "app/context.h"
#include "ui/mainwindow.h"
#include "ThemeManager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    KotobaAppContext appContext;

    QApplication a(argc, argv);
    ThemeManager::setTheme(a, Theme::Dark);  // Set the theme to dark
    MainWindow w(&appContext);
    w.show();
    return a.exec();
}
