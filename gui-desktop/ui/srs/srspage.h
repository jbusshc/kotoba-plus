#pragma once

#include "../../srs/srs_presenter.h"
#include <QWidget>
#include <QShortcut>

namespace Ui {
class SrsPage;
}

class SrsPresenter;

class SrsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SrsPage(KotobaAppContext *ctx, QWidget *parent = nullptr);
    ~SrsPage();

    void setWord(const QString &word);
    void setMeaning(const QString &meaning);
    void resetCard();
    void showNoMoreCards();
    void refreshDashboardStats();
    void refreshStats();
    SrsPresenter* getSrsPresenter() const { return presenter; }

    QShortcut* scAgain;
    QShortcut* scHard;
    QShortcut* scGood;
    QShortcut* scEasy;

    void enableShortcuts(bool enabled);


    // invocado por la UI externa (por ejemplo el dashboard)
    Q_SLOT void startStudy();

signals:
    void showAnswerRequested();
    void againRequested();
    void hardRequested();
    void goodRequested();
    void easyRequested();

    // reenvía contadores para que MainWindow o Dashboard puedan conectar
    void countsUpdated(uint32_t due, uint32_t learning, uint32_t newly, uint32_t lapsed);
    void sessionFinished();

private:
    Ui::SrsPage *ui;
    SrsPresenter* presenter;
};
