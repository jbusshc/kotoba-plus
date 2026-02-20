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
    bool contains(uint32_t entryId) const;
    void add(uint32_t entryId);
    void remove(uint32_t entryId);

public slots:
    void answerAgain();
    void answerHard();
    void answerGood();
    void answerEasy();
    void revealAnswer();

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();

    void showCounts(uint32_t due, uint32_t learning,
                    uint32_t newly, uint32_t lapsed);

    void updateProgress(uint32_t done, uint32_t total);

private:
    void loadNext();
    void handleAnswer(srs_quality grade);
    void autoSave();

    QString snapshotPath;
    KotobaAppContext *context;
    SrsService *service;

    uint32_t currentEntryId = 0;
    kotoba_dict *dict;
    QString currentMeaning;

    // NUEVO MODELO DE SESIÓN
    uint32_t sessionTotal = 0;
    uint32_t sessionRemaining = 0;
};

#endif
