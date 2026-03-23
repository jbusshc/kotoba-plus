#pragma once
#include <QObject>
#include <QString>
#include <QVariantMap>

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
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY statsChanged)

    Q_PROPERTY(QString     currentWord      READ currentWord      NOTIFY currentChanged)
    Q_PROPERTY(QString     currentMeaning   READ currentMeaning   NOTIFY currentChanged)
    Q_PROPERTY(bool        hasCard          READ hasCard          NOTIFY currentChanged)
    Q_PROPERTY(int         currentEntryId   READ currentEntryId   NOTIFY currentChanged)
    Q_PROPERTY(QVariantMap currentEntryData READ currentEntryData NOTIFY currentChanged)

public:
    explicit SrsViewModel(SrsService *service, kotoba_dict *dict,
                          EntryDetailsViewModel *detailsVM,
                          QObject *parent = nullptr);

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

    Q_INVOKABLE bool saveProfile();
    Q_INVOKABLE bool undoLastAnswer();

    bool        canUndo()          const;
    int         totalCount()       const;
    int         dueCount()         const;
    int         learningCount()    const;
    int         newCount()         const;
    int         lapsedCount()      const;
    int         reviewTodayCount() const;

    QString     againInterval()    const;
    QString     hardInterval()     const;
    QString     goodInterval()     const;
    QString     easyInterval()     const;

    QString     currentWord()      const;
    QString     currentMeaning()   const;
    bool        hasCard()          const;
    int         currentEntryId()   const;
    QVariantMap currentEntryData() const;

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();
    void statsChanged();
    void containsChanged(int id);
    void currentChanged();

private:
    void loadNext();
    void updateStats();

    SrsService            *m_service   = nullptr;
    kotoba_dict           *m_dict      = nullptr;
    EntryDetailsViewModel *m_detailsVM = nullptr;

    uint32_t    m_currentEntryId   = 0;
    QString     m_currentWord;
    QString     m_currentMeaning;
    bool        m_hasCard          = false;
    QVariantMap m_currentEntryData;

    struct UndoEntry {
        fsrs_card card;
        uint32_t  entryId;
        uint32_t  cards_done;
        uint32_t  new_done;
        uint32_t  learn_done;
        uint32_t  review_done;
    };
    std::optional<UndoEntry> m_undoStack;
};