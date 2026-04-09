#pragma once

#include <optional>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>

#include <QFuture>

#include "SrsHistoryLog.h"
#include "CardType.h"
#include "CustomDeck.h"
#include "SrsScheduler.h"

extern "C" {
#include <fsrs.h>
#include <fsrs_sync.h>
}

class Configuration;

// ─────────────────────────────────────────────────────────────────────────────
// SrsService — dos decks independientes (Recognition + Recall).
//
// Cada deck usa ent_seq directamente como ID, sin offsets.
// El scheduler (SrsScheduler) intercala cartas de ambos decks.
//
// API pública:
//   - add/remove/contains/reset/suspend/unsuspend trabajan sobre (entSeq, CardType)
//   - nextCard()  → NextCard { fsrs_card*, CardType, entSeq }
//   - answer()    → toma (entSeq, CardType, rating)
//   - CustomDeck sessions: startCustomSession(deckId)
// ─────────────────────────────────────────────────────────────────────────────

class SrsService
{
public:
    explicit SrsService();
    ~SrsService();

    void initialize(uint32_t dictSize, Configuration *config);

    bool load(const QString &basePath);
    bool save();
    void saveAsync();

    // ── Manipulación de cartas ────────────────────────────────────────────────

    // Agrega una carta del tipo indicado.
    // Si autoAddRecall está activo y type == Recognition, también agrega Recall.
    bool add(uint32_t entSeq, CardType type = CardType::Recognition);

    // Agrega ambos tipos en una sola operación (ignora autoAddRecall).
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

    // Sesión normal (ambos decks).
    void startSession(uint32_t limit = 0);

    // Sesión de CustomDeck — solo sirve cartas cuyos ent_seq están en el mazo.
    void startCustomSession(const QString &customDeckId, uint32_t limit = 0);

    // Devuelve la siguiente carta de la sesión activa.
    NextCard nextCard();

    // answer toma el ent_seq + tipo de la carta devuelta por nextCard().
    void answer(uint32_t entSeq, CardType type, fsrs_rating rating);

    bool undoLastAnswer();
    bool canUndo() const { return m_undoStack.has_value(); }

    // ── Estadísticas ──────────────────────────────────────────────────────────

    uint32_t totalCount()       const;   // suma de ambos decks
    uint32_t dueCount()         const;
    uint32_t learningCount()    const;
    uint32_t newCount()         const;
    uint32_t lapsedCount()      const;
    uint32_t reviewTodayCount() const;   // suma de ambas sesiones

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

    std::mutex &deckMutex() { return m_deckMutex; }

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

    fsrs_deck   *deckFor (CardType type) const;
    FsrsSync    &syncFor (CardType type);
    SrsHistoryLog *historyFor(CardType type) const;
    fsrs_session &sessionFor(CardType type);

    fsrs_card *getCard(uint32_t entSeq, CardType type) const;

    // Rutas derivadas del basePath
    std::string srsPath    (CardType type) const;
    std::string snapPath   (CardType type) const;
    std::string logPath    (CardType type) const;
    std::string customDecksPath() const;

    bool compactSync(CardType type, uint64_t now);
    void applyConfig(fsrs_deck *deck) const;

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

    // Filtro activo para sesiones de CustomDeck (vacío = sin filtro)
    QVector<uint32_t>  m_activeFilter;

    uint32_t            m_dictSize     = 0;
    const Configuration *m_config     = nullptr;
    std::string         m_basePath;
    bool                m_autoAddRecall = false;

    mutable std::mutex m_deckMutex;
};