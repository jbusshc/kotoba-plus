#include "SrsScheduler.h"

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────

bool SrsScheduler::inFilter(uint32_t entSeq,
                             const uint32_t *filter,
                             uint32_t        filterSize)
{
    if (!filter || filterSize == 0) return true;
    for (uint32_t i = 0; i < filterSize; ++i)
        if (filter[i] == entSeq) return true;
    return false;
}

// Wraps fsrs_next_for_session con soporte de filtro por ent_seq.
// Si la carta que devolvería el core no pasa el filtro, la saltamos
// respondiendo FSRS_AGAIN con 0 segundos de espera (no muta el estado)
// y buscamos la siguiente. Esto es O(n) en el peor caso pero el filtro
// solo aplica a sesiones de CustomDeck que son subsets pequeños.
fsrs_card *SrsScheduler::peekNext(fsrs_deck    *deck,
                                   fsrs_session *session,
                                   uint64_t      now,
                                   const uint32_t *filter,
                                   uint32_t        filterSize)
{
    if (!deck || !session) return nullptr;

    // Sin filtro: delegamos directamente al core.
    if (!filter || filterSize == 0)
        return fsrs_next_for_session(deck, session, now);

    // Con filtro: iteramos las cartas del deck buscando la primera que:
    //   a) el core serviría (misma lógica de prioridad)
    //   b) está en el filtro
    // Estrategia: pedimos al core y si no pasa el filtro buscamos manualmente
    // entre las cartas due del deck.
    fsrs_card *candidate = fsrs_next_for_session(deck, session, now);
    if (!candidate) return nullptr;
    if (inFilter(candidate->id, filter, filterSize)) return candidate;

    // El candidato no está en el filtro: buscamos en el deck completo.
    uint32_t count = fsrs_deck_count(deck);
    uint64_t today = fsrs_current_day(deck, now);
    fsrs_card *best = nullptr;

    for (uint32_t i = 0; i < count; ++i) {
        const fsrs_card *c = fsrs_deck_card(deck, i);
        if (!c) continue;
        if (c->state == FSRS_STATE_SUSPENDED) continue;
        if (!inFilter(c->id, filter, filterSize)) continue;

        // Solo cartas que serían servibles hoy
        if (c->state == FSRS_STATE_REVIEW && (uint64_t)c->due_day > today) continue;

        if (!best) {
            best = const_cast<fsrs_card *>(c);
            continue;
        }

        // Prioridad: learning primero, luego por due_ts
        bool c_learn  = (c->state    == FSRS_STATE_LEARNING || c->state    == FSRS_STATE_RELEARNING);
        bool best_learn = (best->state == FSRS_STATE_LEARNING || best->state == FSRS_STATE_RELEARNING);

        if (c_learn && !best_learn) { best = const_cast<fsrs_card *>(c); continue; }
        if (!c_learn && best_learn) continue;
        if (c->due_ts < best->due_ts) best = const_cast<fsrs_card *>(c);
    }

    return best;
}

// ─────────────────────────────────────────────────────────────────────────────
// next — scheduler principal
// ─────────────────────────────────────────────────────────────────────────────

NextCard SrsScheduler::next(fsrs_deck        *recognition,
                             fsrs_session     *recogSession,
                             fsrs_deck        *recall,
                             fsrs_session     *recallSession,
                             uint64_t          now,
                             const uint32_t   *customFilter,
                             uint32_t          filterSize)
{
    fsrs_card *rCard  = peekNext(recognition, recogSession, now, customFilter, filterSize);
    fsrs_card *rcCard = peekNext(recall,      recallSession, now, customFilter, filterSize);

    if (!rCard && !rcCard) return {};

    // Solo un deck tiene carta
    if (!rCard) return { rcCard, CardType::Recall,      rcCard->id };
    if (!rcCard) return { rCard,  CardType::Recognition, rCard->id  };

    // Ambos tienen carta — elegimos la más urgente.
    // Regla de prioridad:
    //   1. Learning/Relearning ready (due_ts <= now) gana siempre
    //   2. Empate de tipo: la de due_ts menor
    //   3. Empate exacto: Recognition primero (arbitrario pero consistente)

    auto isLearning = [](const fsrs_card *c) {
        return c->state == FSRS_STATE_LEARNING ||
               c->state == FSRS_STATE_RELEARNING;
    };

    bool rLearn  = isLearning(rCard)  && rCard->due_ts  <= now;
    bool rcLearn = isLearning(rcCard) && rcCard->due_ts <= now;

    if (rLearn && !rcLearn) return { rCard,  CardType::Recognition, rCard->id  };
    if (rcLearn && !rLearn) return { rcCard, CardType::Recall,      rcCard->id };

    // Ambos o ninguno en learning-ready: menor due_ts gana
    if (rCard->due_ts < rcCard->due_ts)
        return { rCard,  CardType::Recognition, rCard->id  };
    if (rcCard->due_ts < rCard->due_ts)
        return { rcCard, CardType::Recall,      rcCard->id };

    // Empate exacto: Recognition primero
    return { rCard, CardType::Recognition, rCard->id };
}