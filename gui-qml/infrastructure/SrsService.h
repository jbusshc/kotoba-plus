#pragma once
#include <optional>
#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

#include <QFuture>

#include "SrsHistoryLog.h"
#include "CardType.h"

extern "C" {
#include <fsrs.h>
#include <fsrs_sync.h>
}

class Configuration;

// ─────────────────────────────────────────────────────────────────────────────
// SrsService — adaptador entre el GUI (ViewModel) y el core FSRS.
//
// Cambios respecto a la versión anterior:
//  - Todas las operaciones públicas aceptan (entryId, CardType).
//    Internamente traducen a fsrsId via CardTypeHelper::toFsrsId().
//  - add():   puede agregar Recognition, Recall, o ambas en una sola llamada.
//  - remove()/contains()/reset(): sobrecargados para operar sobre un tipo
//    específico o sobre ambos a la vez.
//  - nextCard(): devuelve el fsrsId completo; SrsViewModel lo descompone
//    con CardTypeHelper para saber qué mostrar.
//  - autoAddRecall: si está activo, add(Recognition) añade Recall también.
// ─────────────────────────────────────────────────────────────────────────────

class SrsService
{
public:
    explicit SrsService();
    ~SrsService();

    void initialize(uint32_t dictSize, Configuration *config);

    bool load(const char *path);
    bool save(const char *path = nullptr);

    // ── Manipulación de cartas ────────────────────────────────────────────────

    // Agrega una carta del tipo indicado.
    // Si autoAddRecall está activo y type == Recognition, también agrega Recall.
    bool add(uint32_t entryId,
             CardType type = CardType::Recognition);

    // Agrega ambos tipos en una sola operación (ignora autoAddRecall).
    bool addBoth(uint32_t entryId);

    // Elimina la carta del tipo indicado.
    bool remove(uint32_t entryId, CardType type);

    // Elimina ambos tipos si existen.
    void removeBoth(uint32_t entryId);

    bool contains(uint32_t entryId, CardType type) const;

    // Resetea la carta del tipo indicado a estado New.
    bool reset(uint32_t entryId, CardType type);

    bool suspend  (uint32_t entryId, CardType type);
    bool unsuspend(uint32_t entryId, CardType type);
    bool mark     (uint32_t entryId, bool marked, CardType type);
    bool snooze   (uint32_t entryId, uint32_t days, CardType type);

    // ── Sesión / scheduling ───────────────────────────────────────────────────

    void startSession(uint32_t limit = 0);

    // Devuelve el fsrsId completo de la siguiente carta (bit 31 = tipo).
    // El caller usa CardTypeHelper::toEntryId() y ::toCardType() para
    // obtener entryId y CardType por separado.
    // Devuelve nullptr si no hay más cartas en la sesión.
    fsrs_card* nextCard();

    void answer(uint32_t fsrsId, fsrs_rating rating);   // fsrsId completo
    bool undoLastAnswer();
    bool canUndo() const { return m_undoStack.has_value(); }

    // ── Estadísticas ──────────────────────────────────────────────────────────

    uint32_t totalCount()       const;
    uint32_t dueCount()         const;
    uint32_t learningCount()    const;
    uint32_t newCount()         const;
    uint32_t lapsedCount()      const;
    uint32_t reviewTodayCount() const;

    uint64_t waitSeconds() const;

    std::string predictInterval(uint32_t fsrsId, fsrs_rating rating);
    std::string dueDateText    (uint32_t fsrsId) const;
    std::string stateText      (uint32_t fsrsId) const;

    // ── Config ────────────────────────────────────────────────────────────────

    void updateConfig(const Configuration *config);

    // Si es true, add(Recognition) también agrega Recall automáticamente.
    void setAutoAddRecall(bool v) { m_autoAddRecall = v; }
    bool autoAddRecall() const    { return m_autoAddRecall; }

    // ── Acceso interno ────────────────────────────────────────────────────────

    fsrs_deck*    getDeck()    const { return m_deck; }
    fsrs_session* getSession()       { return &m_session; }

    void saveAsync(const char *path = nullptr);
    std::mutex& deckMutex() { return m_deckMutex; }

private:
    // ── Operaciones de bajo nivel (trabajan con fsrsId completo) ─────────────

    bool _add      (uint32_t fsrsId);
    bool _remove   (uint32_t fsrsId);
    bool _contains (uint32_t fsrsId) const;
    bool _reset    (uint32_t fsrsId);
    bool _suspend  (uint32_t fsrsId);
    bool _unsuspend(uint32_t fsrsId);

    fsrs_card* getCard(uint32_t fsrsId) const;
    std::string effectivePath() const;
    bool        compactSync(uint64_t now);

    // ── Estado ────────────────────────────────────────────────────────────────

    struct UndoEntry {
        fsrs_card card;
        uint32_t  fsrsId;       // id completo (con bit de tipo)
        uint32_t  cards_done;
        uint32_t  new_done;
        uint32_t  learn_done;
        uint32_t  review_done;
    };
    std::optional<UndoEntry> m_undoStack;

    fsrs_deck   *m_deck    = nullptr;
    FsrsSync     m_sync    = {};
    fsrs_session m_session = {};

    uint32_t     m_dictSize = 0;
    const Configuration *m_config = nullptr;
    std::string  m_profilePath;
    bool         m_autoAddRecall = false;

    std::unique_ptr<SrsHistoryLog> m_history;
    mutable std::mutex m_deckMutex;
};