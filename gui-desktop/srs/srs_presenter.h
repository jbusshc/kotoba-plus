#ifndef SRS_PRESENTER_H
#define SRS_PRESENTER_H

#include <QObject>
#include "srs_service.h"
#include "../app/context.h"

#ifdef __cplusplus
extern "C" {
    #include "loader.h"
    #include "viewer.h"
}
#endif


class SrsPresenter : public QObject
{
    Q_OBJECT

public:
    explicit SrsPresenter(KotobaAppContext* ctx,
                        QObject* parent = nullptr);


    void startSession();

public slots:
    void answerAgain();
    void answerGood();
    void answerEasy();

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();

private:
    void loadNext();

    KotobaAppContext* context;
    SrsService* service;

    uint32_t currentEntryId = 0;
    kotoba_dict* dict;

};

#endif
