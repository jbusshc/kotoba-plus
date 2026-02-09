#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QModelIndex>
#include <QVector>

#include "app_context.h"
#include "search_presenter.h"
#include "search_service.h"
#include "search_result_model.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(KotobaAppContext *ctx, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSearchTextChanged();
    void onSearchResultClicked(const QModelIndex &index);
    void onBackButtonClicked();
    void showEntry(uint32_t entryId);   // 👈 FALTABA
    
private:
    Ui::MainWindow *ui;

    QTimer *searchTimer;

    SearchPresenter *presenter;
    SearchResultModel *searchResultModel;   
};

#endif // MAINWINDOW_H
