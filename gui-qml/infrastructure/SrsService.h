#pragma once
#include <optional>
#include <string>
#include <cstdint>
#include <vector>
#include "SrsHistoryLog.h"
#include <memory>

extern "C" {
#include <fsrs.h>
#include <fsrs_sync.h>
}

class Configuration;

/*
 * SrsService — adaptador entre el GUI (ViewModel) y el core FSRS.
 *
 * Cambios respecto a la versión anterior:
 *  - effectivePath(): fuente única de verdad para la ruta de perfil.
 *  - nextCard(): card_is_due usa fsrs_current_day() en vez de división cruda,
 *    y el reset de sesión se hace correctamente con la hora actual.
 *  - reset(id): registra evento de sync y compacta.
 *  - reviewTodayCount(): expone m_session.cards_done (cartas respondidas
 *    en la sesión de hoy) en vez del campo due_today de las stats.
 *  - load(): reinicia la sesión con now para que el día lógico sea correcto.
 */
class SrsService
{
public:
    explicit SrsService(uint32_t dictSize, Configuration *config);
    ~SrsService();

    bool load(const char *path);
    bool save(const char *path = nullptr);   // nullptr → usa effectivePath()

    bool add(uint32_t entryId);
    bool remove(uint32_t entryId);
    bool contains(uint32_t entryId) const;
    bool reset(uint32_t entryId);           // nuevo: reset limpio con sync

    bool suspend(uint32_t entryId);
    bool unsuspend(uint32_t entryId);
    bool mark(uint32_t entryId, bool marked);
    bool snooze(uint32_t entryId, uint32_t days);

    void startSession(uint32_t limit = 0);

    fsrs_card* nextCard();
    void answer(uint32_t entryId, fsrs_rating rating);

    /* Estadísticas */
    uint32_t totalCount()    const;   // fsrs_deck_count() — número real de cartas
    uint32_t dueCount()      const;
    uint32_t learningCount() const;
    uint32_t newCount()      const;
    uint32_t lapsedCount()   const;
    uint32_t reviewTodayCount() const;    // cartas respondidas hoy en sesión

    uint64_t waitSeconds() const;

    std::string predictInterval(uint32_t entryId, fsrs_rating rating);

    /* Tiempo en segundos hasta la próxima carta */
    std::string dueDateText(uint32_t entryId) const; // nuevo: texto de vencimiento real
    std::string stateText(uint32_t entryId) const;   // nuevo: texto del estado (New, Learning, Review, etc.)

    fsrs_deck*    getDeck()    const { return m_deck; }
    fsrs_session* getSession()       { return &m_session; }
    
    bool undoLastAnswer();
    bool canUndo() const { return m_undoStack.has_value(); }

private:

    struct UndoEntry {
        fsrs_card    card;          // copia completa de la carta antes de responder
        uint32_t     entryId;
        uint32_t     cards_done;
        uint32_t     new_done;
        uint32_t     learn_done;
        uint32_t     review_done;
        // La palabra/meaning que se estaba mostrando (para relanzar showQuestion)
    };
    std::optional<UndoEntry> m_undoStack;

    fsrs_card* getCard(uint32_t entryId) const;
    std::string effectivePath() const;   // fuente única de verdad para la ruta
    bool        compactSync(uint64_t now);

    fsrs_deck   *m_deck    = nullptr;
    FsrsSync     m_sync    = {};
    fsrs_session m_session = {};

    uint32_t     m_dictSize = 0;
    Configuration *m_config = nullptr;
    std::string  m_profilePath;

    std::unique_ptr<SrsHistoryLog> m_history;
};