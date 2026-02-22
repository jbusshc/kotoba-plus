#include "app/context.h"
#include "ui/mainwindow.h"
#include "ThemeManager.h"

#include <QApplication>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    a.setStyle("Fusion");
    a.setApplicationName("Kotoba+");
    ThemeManager::setTheme(a, Theme::Dark);  // Set the theme to dark or light

    KotobaAppContext appContext;
    MainWindow w(&appContext);
    w.show();
    return a.exec();
}
