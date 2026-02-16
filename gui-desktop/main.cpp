#include "app/context.h"
#include "ui/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    KotobaAppContext appContext;

    QApplication a(argc, argv);
    MainWindow w(&appContext);
    w.show();
    return a.exec();
}
