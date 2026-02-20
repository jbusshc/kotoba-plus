#include "srspage.h"
#include "ui_srspage.h"

#include <QShortcut>
#include <QKeySequence>

SrsPage::SrsPage(KotobaAppContext *ctx, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::SrsPage)
{
    ui->setupUi(this);

    presenter = new SrsPresenter(ctx, this);

    // Conexiones presenter -> view
    connect(presenter, &SrsPresenter::showQuestion,
            this, [this](const QString &word){
                resetCard();
                setWord(word);
            });

    connect(presenter, &SrsPresenter::showAnswer,
            this, &SrsPage::setMeaning);


    // reenviar contadores hacia el exterior (ej. MainWindow -> Dashboard)
    connect(presenter, &SrsPresenter::showCounts,
            this, [this](uint32_t due, uint32_t learning, uint32_t newly, uint32_t lapsed){
                emit countsUpdated(due, learning, newly, lapsed);
            });

    // progreso
    connect(presenter, &SrsPresenter::updateProgress,
            this, [this](uint32_t done, uint32_t total){
                ui->labelProgress->setText(QString("%1 / %2").arg(done).arg(total));
                ui->progressBar->setMaximum(total > 0 ? total : 1);
                ui->progressBar->setValue(done);
            });

    // Conexiones view -> presenter (respuestas)
    connect(this, &SrsPage::againRequested,
            presenter, &SrsPresenter::answerAgain);

    connect(this, &SrsPage::hardRequested,
            presenter, &SrsPresenter::answerHard);

    connect(this, &SrsPage::goodRequested,
            presenter, &SrsPresenter::answerGood);

    connect(this, &SrsPage::easyRequested,
            presenter, &SrsPresenter::answerEasy);

    // Conectar petición de mostrar respuesta al presenter
    connect(this, &SrsPage::showAnswerRequested,
            presenter, &SrsPresenter::revealAnswer);

    // Ocultar respuesta y deshabilitar visualizacion de botones de valoración al inicio
    ui->labelAnswer->setVisible(false);
    ui->btnAgain->setVisible(false);
    ui->btnHard->setVisible(false);
    ui->btnGood->setVisible(false);
    ui->btnEasy->setVisible(false);

    connect(ui->btnShowAnswer, &QPushButton::clicked, this, [=]() {

        ui->labelAnswer->setVisible(true);

        ui->btnShowAnswer->setVisible(false);

        ui->btnAgain->setVisible(true);
        ui->btnHard->setVisible(true);
        ui->btnGood->setVisible(true);
        ui->btnEasy->setVisible(true);

        enableShortcuts(true);

        emit showAnswerRequested();
    });



    // conectar botones de valoración
    connect(ui->btnAgain, &QPushButton::clicked,
            this, &SrsPage::againRequested);

    connect(ui->btnHard, &QPushButton::clicked,
            this, &SrsPage::hardRequested);

    connect(ui->btnGood, &QPushButton::clicked,
            this, &SrsPage::goodRequested);

    connect(ui->btnEasy, &QPushButton::clicked,
            this, &SrsPage::easyRequested);

    // Atajos teclado: 1=A, 2=H, 3=G, 4=E
    scAgain = new QShortcut(QKeySequence(Qt::Key_1), this);
    scHard  = new QShortcut(QKeySequence(Qt::Key_2), this);
    scGood  = new QShortcut(QKeySequence(Qt::Key_3), this);
    scEasy  = new QShortcut(QKeySequence(Qt::Key_4), this);

    connect(scAgain, &QShortcut::activated, this, [=](){ ui->btnAgain->click(); });
    connect(scHard,  &QShortcut::activated, this, [=](){ ui->btnHard->click(); });
    connect(scGood,  &QShortcut::activated, this, [=](){ ui->btnGood->click(); });
    connect(scEasy,  &QShortcut::activated, this, [=](){ ui->btnEasy->click(); });

    enableShortcuts(false);


    connect(presenter, &SrsPresenter::noMoreCards,
        this, [this]() {
            showNoMoreCards();
            emit sessionFinished();
        });


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
    ui->labelAnswer->clear();
    ui->labelAnswer->setVisible(false);

    ui->btnAgain->setVisible(false);
    ui->btnHard->setVisible(false);
    ui->btnGood->setVisible(false);
    ui->btnEasy->setVisible(false);

    ui->btnAgain->setEnabled(true);
    ui->btnHard->setEnabled(true);
    ui->btnGood->setEnabled(true);
    ui->btnEasy->setEnabled(true);

    ui->btnShowAnswer->setVisible(true);
    ui->btnShowAnswer->setEnabled(true);

    enableShortcuts(false);
}



void SrsPage::showNoMoreCards()
{
    ui->labelWord->setText("No hay más tarjetas para hoy");

    ui->labelAnswer->clear();
    ui->labelAnswer->setVisible(false);

    ui->btnAgain->setVisible(false);
    ui->btnHard->setVisible(false);
    ui->btnGood->setVisible(false);
    ui->btnEasy->setVisible(false);

    ui->btnShowAnswer->setVisible(false);

    enableShortcuts(false);
}


void SrsPage::startStudy()
{
    presenter->startSession();
}

void SrsPage::refreshDashboardStats()
{
    presenter->startSession();
}

void SrsPage::refreshStats()
{
    presenter->refreshStats();
}

void SrsPage::enableShortcuts(bool enabled)
{
    scAgain->setEnabled(enabled);
    scHard->setEnabled(enabled);
    scGood->setEnabled(enabled);
    scEasy->setEnabled(enabled);
}
