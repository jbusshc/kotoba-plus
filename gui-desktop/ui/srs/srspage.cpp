#include "srspage.h"
#include "ui_srspage.h"

#include "../../srs/srs_presenter.h"

SrsPage::SrsPage(KotobaAppContext *ctx, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::SrsPage)
{
    ui->setupUi(this);

    presenter = new SrsPresenter(ctx, this);

    connect(presenter, &SrsPresenter::showQuestion,
            this, &SrsPage::setWord);

    connect(presenter, &SrsPresenter::showAnswer,
            this, &SrsPage::setMeaning);

    connect(presenter, &SrsPresenter::noMoreCards,
            this, &SrsPage::showNoMoreCards);

    connect(this, &SrsPage::againRequested,
            presenter, &SrsPresenter::answerAgain);

    connect(this, &SrsPage::goodRequested,
            presenter, &SrsPresenter::answerGood);

    connect(this, &SrsPage::easyRequested,
            presenter, &SrsPresenter::answerEasy);
    // Ocultar respuesta al inicio
    ui->labelAnswer->setVisible(false);

    // Conectar botones UI → señales
    connect(ui->btnShowAnswer, &QPushButton::clicked,
            this, [=]()
            {
        ui->labelAnswer->setVisible(true);
        emit showAnswerRequested(); });

    connect(ui->btnAgain, &QPushButton::clicked,
            this, &SrsPage::againRequested);

    connect(ui->btnGood, &QPushButton::clicked,
            this, &SrsPage::goodRequested);

    connect(ui->btnEasy, &QPushButton::clicked,
            this, &SrsPage::easyRequested);

}

SrsPage::~SrsPage()
{
    delete ui;
}

void SrsPage::setWord(const QString &word)
{
    ui->labelWord->setText(word);
}

void SrsPage::setMeaning(const QString &meaning)
{
    ui->labelAnswer->setText(meaning);
}

void SrsPage::resetCard()
{
    ui->labelAnswer->setVisible(false);
}

void SrsPage::showNoMoreCards()
{
    ui->labelWord->setText("No hay más tarjetas para hoy 🎉");
    ui->labelAnswer->clear();
    ui->labelAnswer->setVisible(false);
}


