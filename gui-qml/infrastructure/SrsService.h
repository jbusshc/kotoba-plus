#pragma once

#include <optional>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>

#include <QString>
#include <QVector>

#include "SrsHistoryLog.h"
#include "CardType.h"
#include "CustomDeck.h"
#include "SrsScheduler.h"

extern "C" {
#include <fsrs.h>
#include <fsrs_sync.h>
}

class Configuration;

class SrsService
{
public:
    explicit SrsService();
    ~SrsService();

    void initialize(uint32_t dictSize, Configuration *config);

    // basePath es el prefijo sin extensión, p.ej. ".../profile"
    bool load(const QString &basePath);
    bool save();
    void saveAsync();

    // ── Manipulación de cartas ────────────────────────────────────────────────

    bool add(uint32_t entSeq, CardType type = CardType::Recognition);
    bool addBoth(uint32_t entSeq);
    bool remove  (uint32_t entSeq, CardType type);
    void removeBoth(uint32_t entSeq);
    bool contains(uint32_t entSeq, CardType type) const;
    bool reset    (uint32_t entSeq, CardType type);
    bool suspend  (uint32_t entSeq, CardType type);
    bool unsuspend(uint32_t entSeq, CardType type);
    bool mark     (uint32_t entSeq, bool marked, CardType type);
    bool snooze   (uint32_t entSeq, uint32_t days, CardType type);

    // ── Sesión / scheduling ───────────────────────────────────────────────────

    void startSession(uint32_t limit = 0);
    void startCustomSession(const QString &customDeckId, uint32_t limit = 0);

    NextCard nextCard();
    void answer(uint32_t entSeq, CardType type, fsrs_rating rating);
    bool undoLastAnswer();
    bool canUndo() const { return m_undoStack.has_value(); }

    // ── Estadísticas ──────────────────────────────────────────────────────────

    uint32_t totalCount()       const;
    uint32_t dueCount()         const;
    uint32_t learningCount()    const;
    uint32_t newCount()         const;
    uint32_t lapsedCount()      const;
    // Suma cards_done de ambas sesiones — persistido entre reinicios
    uint32_t reviewTodayCount() const;

    uint64_t waitSeconds() const;

    std::string predictInterval(uint32_t entSeq, CardType type, fsrs_rating rating);
    std::string dueDateText    (uint32_t entSeq, CardType type) const;
    std::string stateText      (uint32_t entSeq, CardType type) const;

    // ── Config ────────────────────────────────────────────────────────────────

    void updateConfig(const Configuration *config);
    void setAutoAddRecall(bool v) { m_autoAddRecall = v; }
    bool autoAddRecall()    const { return m_autoAddRecall; }

    // ── Acceso interno ────────────────────────────────────────────────────────

    fsrs_deck *recognitionDeck() const { return m_recognition; }
    fsrs_deck *recallDeck()      const { return m_recall; }
    std::mutex &deckMutex()            { return m_deckMutex; }

    // ── CustomDecks ───────────────────────────────────────────────────────────

    CustomDeckRegistry       &customDecks()       { return m_customDecks; }
    const CustomDeckRegistry &customDecks() const { return m_customDecks; }
    bool saveCustomDecks() { return m_customDecks.save(); }

private:
    // ── Operaciones de bajo nivel ─────────────────────────────────────────────

    bool _add      (uint32_t entSeq, CardType type);
    bool _remove   (uint32_t entSeq, CardType type);
    bool _reset    (uint32_t entSeq, CardType type);
    bool _suspend  (uint32_t entSeq, CardType type);
    bool _unsuspend(uint32_t entSeq, CardType type);

    fsrs_deck    *deckFor    (CardType type) const;
    FsrsSync     &syncFor    (CardType type);
    SrsHistoryLog *historyFor(CardType type) const;
    fsrs_session  &sessionFor(CardType type);

    fsrs_card *getCard(uint32_t entSeq, CardType type) const;

    // Rutas derivadas del basePath
    std::string srsPath         (CardType type) const;
    std::string snapPath        (CardType type) const;
    std::string logPath         (CardType type) const;
    std::string sessionPath     ()              const;  // <base>.session.json
    std::string customDecksPath ()              const;

    bool compactSync(CardType type, uint64_t now);
    void applyConfig(fsrs_deck *deck) const;

    // Persiste/restaura cards_done del día actual para reviewTodayCount
    void saveSession() const;
    void loadSession();

    // ── Estado ────────────────────────────────────────────────────────────────

    struct UndoEntry {
        fsrs_card card;
        uint32_t  entSeq;
        CardType  type;
        uint32_t  cards_done;
        uint32_t  new_done;
        uint32_t  learn_done;
        uint32_t  review_done;
    };
    std::optional<UndoEntry> m_undoStack;

    fsrs_deck   *m_recognition  = nullptr;
    fsrs_deck   *m_recall       = nullptr;

    FsrsSync     m_recogSync    = {};
    FsrsSync     m_recallSync   = {};

    fsrs_session m_recogSession  = {};
    fsrs_session m_recallSession = {};

    std::unique_ptr<SrsHistoryLog> m_recogHistory;
    std::unique_ptr<SrsHistoryLog> m_recallHistory;

    CustomDeckRegistry m_customDecks;
    QVector<uint32_t>  m_activeFilter;

    uint32_t            m_dictSize      = 0;
    const Configuration *m_config      = nullptr;
    std::string          m_basePath;
    bool                 m_autoAddRecall = false;

    mutable std::mutex m_deckMutex;
};