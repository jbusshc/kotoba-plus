#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>

#include "types.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSearchTextChanged();
    void onSearchResultClicked(const QModelIndex &index);
    void onBackButtonClicked();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *searchResultModel;
    QTimer *searchTimer;
};

#endif // MAINWINDOW_H
