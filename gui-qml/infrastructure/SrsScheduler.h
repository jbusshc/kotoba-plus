#pragma once

#include <cstdint>
#include "CardType.h"

extern "C" {
#include "../../core/include/fsrs.h"
}

// ─────────────────────────────────────────────────────────────────────────────
// NextCard — resultado del scheduler.
// ─────────────────────────────────────────────────────────────────────────────

struct NextCard {
    fsrs_card *card    = nullptr;
    CardType   type    = CardType::Recognition;
    uint32_t   entSeq  = 0;       // == card->id

    explicit operator bool() const { return card != nullptr; }
};

// ─────────────────────────────────────────────────────────────────────────────
// SrsScheduler — intercala cartas de dos decks con prioridad compartida.
//
// Prioridad (igual que el core, pero entre decks):
//   1. Learning/Relearning listo (due_ts <= now)   — el más urgente de ambos
//   2. Reviews / New según order_mode               — alternando decks
//   3. Learning pendiente (due_ts > now)            — fallback
//
// El scheduler opera de forma stateless por diseño: no guarda qué carta se
// sirvió antes. El estado de sesión (cards_done, límites) vive en
// fsrs_session y se pasa desde SrsService.
// ─────────────────────────────────────────────────────────────────────────────

class SrsScheduler
{
public:
    // Devuelve la siguiente carta considerando ambos decks.
    // Pasa nullptr en cualquiera de los decks para ignorarlo (p.ej. solo Recall).
    // Si customFilter != nullptr solo sirve cartas cuyo ent_seq está en ese set.
    static NextCard next(fsrs_deck        *recognition,
                         fsrs_session     *recogSession,
                         fsrs_deck        *recall,
                         fsrs_session     *recallSession,
                         uint64_t          now,
                         const uint32_t   *customFilter   = nullptr,
                         uint32_t          filterSize     = 0);

private:
    // Devuelve la siguiente carta de un deck bajo una sesión, respetando el
    // filtro si se proporciona. Wrapper fino sobre fsrs_next_for_session.
    static fsrs_card *peekNext(fsrs_deck    *deck,
                               fsrs_session *session,
                               uint64_t      now,
                               const uint32_t *filter,
                               uint32_t        filterSize);

    static bool inFilter(uint32_t entSeq,
                         const uint32_t *filter,
                         uint32_t        filterSize);
};