#pragma once
#include <QObject>
#include <QString>

class SrsService;
extern "C" {
#include "../../core/include/loader.h"
}

class SrsViewModel : public QObject
{
    Q_OBJECT
public:
    explicit SrsViewModel(SrsService *service, kotoba_dict *dict, QObject *parent = nullptr);

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void revealAnswer();
    Q_INVOKABLE void answerAgain();
    Q_INVOKABLE void answerHard();
    Q_INVOKABLE void answerGood();
    Q_INVOKABLE void answerEasy();

    Q_INVOKABLE void handleAnswer(int quality);

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();

private:
    SrsService *m_service;
    kotoba_dict *m_dict;
    uint32_t m_currentEntryId = 0;
    QString m_currentMeaning;

    void loadNext();
};