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

    Q_PROPERTY(int totalCount      READ totalCount      NOTIFY statsChanged)
    Q_PROPERTY(int dueCount        READ dueCount        NOTIFY statsChanged)
    Q_PROPERTY(int learningCount   READ learningCount   NOTIFY statsChanged)
    Q_PROPERTY(int newCount        READ newCount        NOTIFY statsChanged)
    Q_PROPERTY(int lapsedCount     READ lapsedCount     NOTIFY statsChanged)
    Q_PROPERTY(int reviewTodayCount READ reviewTodayCount NOTIFY statsChanged)

    Q_PROPERTY(QString againInterval READ againInterval NOTIFY statsChanged)
    Q_PROPERTY(QString hardInterval  READ hardInterval  NOTIFY statsChanged)
    Q_PROPERTY(QString goodInterval  READ goodInterval  NOTIFY statsChanged)
    Q_PROPERTY(QString easyInterval  READ easyInterval  NOTIFY statsChanged)

public:
    explicit SrsViewModel(SrsService *service, kotoba_dict *dict, QObject *parent = nullptr);

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void revealAnswer();

    Q_INVOKABLE void answerAgain();
    Q_INVOKABLE void answerHard();
    Q_INVOKABLE void answerGood();
    Q_INVOKABLE void answerEasy();

    Q_INVOKABLE void handleAnswer(int quality);

    Q_INVOKABLE bool contains(int entryId);
    Q_INVOKABLE void add(int entryId);
    Q_INVOKABLE void remove(int entryId);

    /* Guardar el perfil explícitamente (llamado desde QML al salir de estudio) */
    Q_INVOKABLE bool saveProfile();

    int     totalCount()       const;
    int     dueCount()         const;
    int     learningCount()    const;
    int     newCount()         const;
    int     lapsedCount()      const;
    int     reviewTodayCount() const;

    QString againInterval()    const;
    QString hardInterval()     const;
    QString goodInterval()     const;
    QString easyInterval()     const;

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();
    void statsChanged();
    void containsChanged(int id);
    
private:
    void loadNext();
    void updateStats();

    SrsService   *m_service;
    kotoba_dict  *m_dict;

    uint32_t m_currentEntryId = 0;
    QString  m_currentMeaning;
    bool     m_hasCard = false;
};