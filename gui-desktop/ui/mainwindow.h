#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "../app/context.h"
#include "dictionary/dictionarypage.h"
#include "details/detailspage.h"
#include "srs/srspage.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(KotobaAppContext *ctx, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    DictionaryPage *dictionaryPage;
    DetailsPage *detailsPage;
    SrsPage *srsPage;

};

#endif
