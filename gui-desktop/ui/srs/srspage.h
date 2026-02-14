#pragma once

#include "../../srs/srs_presenter.h"


#include <QWidget>

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

signals:
    void showAnswerRequested();
    void againRequested();
    void goodRequested();
    void easyRequested();

private:
    Ui::SrsPage *ui;
    SrsPresenter* presenter;
};
