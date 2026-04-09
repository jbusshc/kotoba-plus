#pragma once
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "../infrastructure/CardType.h"
#include "../infrastructure/SrsScheduler.h"

class SrsService;
class EntryDetailsViewModel;

extern "C" {
#include "../../core/include/loader.h"
#include "../../core/include/fsrs.h"
}

class SrsViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int totalCount       READ totalCount       NOTIFY statsChanged)
    Q_PROPERTY(int dueCount         READ dueCount         NOTIFY statsChanged)
    Q_PROPERTY(int learningCount    READ learningCount    NOTIFY statsChanged)
    Q_PROPERTY(int newCount         READ newCount         NOTIFY statsChanged)
    Q_PROPERTY(int lapsedCount      READ lapsedCount      NOTIFY statsChanged)
    Q_PROPERTY(int reviewTodayCount READ reviewTodayCount NOTIFY statsChanged)

    Q_PROPERTY(QString againInterval READ againInterval NOTIFY statsChanged)
    Q_PROPERTY(QString hardInterval  READ hardInterval  NOTIFY statsChanged)
    Q_PROPERTY(QString goodInterval  READ goodInterval  NOTIFY statsChanged)
    Q_PROPERTY(QString easyInterval  READ easyInterval  NOTIFY statsChanged)
    Q_PROPERTY(bool    canUndo       READ canUndo       NOTIFY statsChanged)

    Q_PROPERTY(QString     currentWord      READ currentWord      NOTIFY currentChanged)
    Q_PROPERTY(QString     currentMeaning   READ currentMeaning   NOTIFY currentChanged)
    Q_PROPERTY(bool        hasCard          READ hasCard          NOTIFY currentChanged)
    // ent_seq de la entrada en curso — estable entre regeneraciones del dict
    Q_PROPERTY(int         currentEntryId   READ currentEntryId   NOTIFY currentChanged)
    Q_PROPERTY(QVariantMap currentEntryData READ currentEntryData NOTIFY currentChanged)
    // "Recognition" o "Recall"
    Q_PROPERTY(QString     currentCardType  READ currentCardType  NOTIFY currentChanged)

public:
    explicit SrsViewModel(QObject *parent = nullptr);

    void initialize(SrsService *service, kotoba_dict *dict, EntryDetailsViewModel *detailsVM);

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void startCustomSession(const QString &customDeckId);
    Q_INVOKABLE void revealAnswer();

    Q_INVOKABLE void answerAgain();
    Q_INVOKABLE void answerHard();
    Q_INVOKABLE void answerGood();
    Q_INVOKABLE void answerEasy();
    Q_INVOKABLE void handleAnswer(int quality);

    // ── Gestión de cartas (ent_seq) ──────────────────────────────────────────

    Q_INVOKABLE bool containsRecognition(int entSeq) const;
    Q_INVOKABLE bool containsRecall     (int entSeq) const;

    Q_INVOKABLE void add      (int entSeq);
    Q_INVOKABLE void addRecall(int entSeq);
    Q_INVOKABLE void addBoth  (int entSeq);

    Q_INVOKABLE void remove      (int entSeq);
    Q_INVOKABLE void removeRecall(int entSeq);
    Q_INVOKABLE void removeBoth  (int entSeq);

    Q_INVOKABLE bool saveProfile();
    Q_INVOKABLE bool undoLastAnswer();

    // ── Propiedades ──────────────────────────────────────────────────────────

    bool    canUndo()          const;
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

    QString     currentWord()      const;
    QString     currentMeaning()   const;
    bool        hasCard()          const;
    int         currentEntryId()   const;
    QVariantMap currentEntryData() const;
    QString     currentCardType()  const;

    void refresh();

signals:
    void noMoreCards();
    void statsChanged();
    // entSeq + cardTypeMask: 1=Recognition, 2=Recall, 3=ambos
    void containsChanged(int entSeq, int cardTypeMask);
    void currentChanged();

private:
    void loadNext();
    void updateStats();

    SrsService            *m_service   = nullptr;
    kotoba_dict           *m_dict      = nullptr;
    EntryDetailsViewModel *m_detailsVM = nullptr;

    // carta en curso
    NextCard    m_current;
    QString     m_currentWord;
    QString     m_currentMeaning;
    bool        m_hasCard          = false;
    QVariantMap m_currentEntryData;
};