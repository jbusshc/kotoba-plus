#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QTimer>
#include "libtango.h"  // Aquí están entry y TangoSearchResult

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Firma consistente con tu .cpp
    void onSearchTextChanged();
    void onSearchResultClicked(const QModelIndex &index);
    void onBackButtonClicked();

private:
    Ui::MainWindow *ui;
    TangoDB *db;
    QStandardItemModel *searchResultModel;
    QTimer *searchTimer;               // temporizador para debounce
    entry *currentEntry;               // entrada actual

    void handleResult(const TangoSearchResult* result);
    void showEntryDetails(const entry* e);
};

#endif // MAINWINDOW_H
