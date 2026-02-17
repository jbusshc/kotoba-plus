#ifndef SRS_PRESENTER_H
#define SRS_PRESENTER_H

#include <QObject>
#include "srs_service.h"
#include "../app/context.h"

#ifdef __cplusplus
extern "C"
{
#include "loader.h"
#include "viewer.h"
}
#endif

class SrsPresenter : public QObject
{
    Q_OBJECT

public:
    explicit SrsPresenter(KotobaAppContext *ctx,
                          QObject *parent = nullptr);

    void startSession();
    void refreshStats();

public slots:
    void answerAgain();
    void answerHard();
    void answerGood();
    void answerEasy();

    void revealAnswer(); // revelar la respuesta cuando se pida

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();

    // enviar contadores al dashboard
    void showCounts(uint32_t due, uint32_t learning, uint32_t newly, uint32_t lapsed);

    // progreso: hecho, total
    void updateProgress(uint32_t done, uint32_t total);

private:
    void loadNext();

    KotobaAppContext *context;
    SrsService *service;

    uint32_t currentEntryId = 0;
    kotoba_dict *dict;
    QString currentMeaning; // almacenar la respuesta hasta revelarla
    uint32_t totalDue = 0;
    uint32_t studied = 0;
};

#endif
