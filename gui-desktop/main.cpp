#include "app_context.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    KotobaAppContext appContext;

    QApplication a(argc, argv);
    MainWindow w(&appContext);
    w.show();
    return a.exec();
}
