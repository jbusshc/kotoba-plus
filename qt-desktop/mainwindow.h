#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QTimer>
#include "libkotobaplus.h"  // Aquí están entry y KPSearchResult
#include "listitemdelegate.h"

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
    KPDB *db;
    QStandardItemModel *searchResultModel;
    QTimer *searchTimer;               // temporizador para debounce
    entry *currentEntry;               // entrada actual

    void handleResult(const KPSearchResult* result);
    void showEntryDetails(const entry* e);
};

#endif // MAINWINDOW_H
