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

    // Ocultar respuesta y deshabilitar botones de valoración al inicio
    ui->labelAnswer->setVisible(false);
    ui->btnAgain->setEnabled(false);
    ui->btnHard->setEnabled(false);
    ui->btnGood->setEnabled(false);
    ui->btnEasy->setEnabled(false);

    // Conectar botón Mostrar respuesta
    connect(ui->btnShowAnswer, &QPushButton::clicked, this, [=]() {
        ui->labelAnswer->setVisible(true);
        // activar botones de valoración cuando la respuesta ya es visible
        ui->btnAgain->setEnabled(true);
        ui->btnHard->setEnabled(true);
        ui->btnGood->setEnabled(true);
        ui->btnEasy->setEnabled(true);
        ui->btnShowAnswer->setEnabled(false); // evitar doble click
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
    auto makeShortcut = [&](const QKeySequence &seq, std::function<void()> cb){
        QShortcut *s = new QShortcut(seq, this);
        connect(s, &QShortcut::activated, this, cb);
    };

    makeShortcut(QKeySequence(Qt::Key_1), [=](){ if (ui->btnAgain->isEnabled()) ui->btnAgain->click(); });
    makeShortcut(QKeySequence(Qt::Key_2), [=](){ if (ui->btnHard->isEnabled()) ui->btnHard->click(); });
    makeShortcut(QKeySequence(Qt::Key_3), [=](){ if (ui->btnGood->isEnabled()) ui->btnGood->click(); });
    makeShortcut(QKeySequence(Qt::Key_4), [=](){ if (ui->btnEasy->isEnabled()) ui->btnEasy->click(); });

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
    ui->labelAnswer->setVisible(false);
    ui->labelAnswer->clear();
    ui->btnAgain->setEnabled(false);
    ui->btnHard->setEnabled(false);
    ui->btnGood->setEnabled(false);
    ui->btnEasy->setEnabled(false);
    ui->btnShowAnswer->setEnabled(true);
}

void SrsPage::showNoMoreCards()
{
    ui->labelWord->setText("No hay más tarjetas para hoy");
    ui->labelAnswer->clear();
    ui->labelAnswer->setVisible(false);
    ui->btnAgain->setEnabled(false);
    ui->btnHard->setEnabled(false);
    ui->btnGood->setEnabled(false);
    ui->btnEasy->setEnabled(false);
    ui->btnShowAnswer->setEnabled(false);
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
