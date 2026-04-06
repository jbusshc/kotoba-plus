#pragma once
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "../infrastructure/CardType.h"

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
    Q_PROPERTY(int         currentEntryId   READ currentEntryId   NOTIFY currentChanged)
    Q_PROPERTY(QVariantMap currentEntryData READ currentEntryData NOTIFY currentChanged)

    // Tipo de la carta en curso: "Recognition" o "Recall"
    // El QML lo usa para mostrar el prompt correcto
    Q_PROPERTY(QString     currentCardType  READ currentCardType  NOTIFY currentChanged)

public:
    explicit SrsViewModel(QObject *parent = nullptr);

    void initialize(SrsService *service, kotoba_dict *dict, EntryDetailsViewModel *detailsVM);

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void revealAnswer();

    Q_INVOKABLE void answerAgain();
    Q_INVOKABLE void answerHard();
    Q_INVOKABLE void answerGood();
    Q_INVOKABLE void answerEasy();
    Q_INVOKABLE void handleAnswer(int quality);

    // ── Gestión de cartas ────────────────────────────────────────────────────

    // ¿Tiene carta de Recognition en el deck?
    Q_INVOKABLE bool containsRecognition(int entryId) const;

    // ¿Tiene carta de Recall en el deck?
    Q_INVOKABLE bool containsRecall(int entryId) const;

    // Agrega Recognition (+ Recall automático si autoAddRecall está activo)
    Q_INVOKABLE void add(int entryId);

    // Agrega solo Recall (e.g. desde el botón manual en DetailsPage)
    Q_INVOKABLE void addRecall(int entryId);

    // Agrega ambos tipos explícitamente
    Q_INVOKABLE void addBoth(int entryId);

    // Elimina Recognition (no toca Recall)
    Q_INVOKABLE void remove(int entryId);

    // Elimina Recall (no toca Recognition)
    Q_INVOKABLE void removeRecall(int entryId);

    // Elimina ambos tipos
    Q_INVOKABLE void removeBoth(int entryId);

    Q_INVOKABLE bool saveProfile();
    Q_INVOKABLE bool undoLastAnswer();

    // ── Propiedades ──────────────────────────────────────────────────────────

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
    QString     currentCardType()  const;

    void refresh();

signals:
    void showQuestion(QString word);
    void showAnswer(QString meaning);
    void noMoreCards();
    void statsChanged();

    // Emitido cuando cambia el estado SRS de una entry concreta.
    // Los tipos se pasan para que el caller sepa cuál cambió.
    void containsChanged(int entryId, int cardTypeMask);  // mask: 1=Recognition, 2=Recall, 3=ambos

    void currentChanged();

private:
    void loadNext();
    void updateStats();

    SrsService            *m_service   = nullptr;
    kotoba_dict           *m_dict      = nullptr;
    EntryDetailsViewModel *m_detailsVM = nullptr;

    // fsrsId completo de la carta en curso (tiene el bit de tipo)
    uint32_t    m_currentFsrsId    = 0;
    QString     m_currentWord;
    QString     m_currentMeaning;
    bool        m_hasCard          = false;
    QVariantMap m_currentEntryData;
};