#include "SrsService.h"
#include "../app/Configuration.h"

#include <string>
#include <cstring>

/* ---- path helpers ---- */

static inline std::string snapshot_path_for(const std::string &base)
{
    return base + ".sync.snap";
}

static inline std::string log_path_for(const std::string &base)
{
    return base + ".sync.log";
}

/* ---- ctor / dtor ---- */

SrsService::SrsService(uint32_t dictSize, Configuration *config)
    : m_dictSize(dictSize), m_config(config)
{
    fsrs_params params = fsrs_default_params();
    m_deck = fsrs_deck_create(dictSize, &params);

    if (m_config)
        fsrs_seed(m_config->deviceId);

    uint64_t device_id = m_config ? m_config->deviceId : 0;
    fsrs_sync_init(&m_sync, device_id);

    /* Sesión por defecto arranca con el timestamp actual para que
       fsrs_current_day() devuelva el día lógico correcto desde el inicio. */
    fsrs_session_start(&m_session, fsrs_now(), 0);
}

SrsService::~SrsService()
{
    fsrs_sync_free(&m_sync);
    if (m_deck)
        fsrs_deck_free(m_deck);
}

/* ---- helpers privados ---- */

fsrs_card* SrsService::getCard(uint32_t entryId) const
{
    if (!m_deck) return nullptr;
    return fsrs_get_card(m_deck, entryId);
}

/*
 * effectivePath() — fuente única de verdad.
 *
 * Prioridad: m_profilePath (establecido en load()) >
 *            m_config->srsPath (ruta por defecto de configuración) >
 *            "" (sin ruta).
 *
 * Antes, cada método copiaba esta lógica de forma inconsistente.
 */
std::string SrsService::effectivePath() const
{
    if (!m_profilePath.empty())
        return m_profilePath;
    if (m_config && !m_config->srsPath.isEmpty())
        return m_config->srsPath.toStdString();
    return {};
}

bool SrsService::compactSync(uint64_t now)
{
    std::string base = effectivePath();
    if (base.empty()) return false;

    std::string snap = snapshot_path_for(base);
    std::string log  = log_path_for(base);

    return fsrs_sync_compact(&m_sync, m_deck, snap.c_str(), log.c_str(), now);
}

/* ---- persistencia ---- */

bool SrsService::load(const char *path)
{
    if (!path) return false;

    fsrs_params params = fsrs_default_params();
    fsrs_deck *deck = fsrs_load(path, &params);
    if (!deck) return false;

    if (m_deck) fsrs_deck_free(m_deck);
    m_deck = deck;
    m_profilePath = path;

    std::string snap = snapshot_path_for(m_profilePath);
    std::string log  = log_path_for(m_profilePath);

    (void)fsrs_sync_snapshot_load(m_deck, snap.c_str(), fsrs_now());
    (void)fsrs_sync_log_load(&m_sync, log.c_str());
    fsrs_sync_rebuild(&m_sync, m_deck, fsrs_now());
    fsrs_rebuild_queues(m_deck, fsrs_now());

    /* BUG FIX: reiniciar la sesión con now para que el día lógico sea el
       correcto desde el primer nextCard(). Antes se usaba el session_start
       del ctor (que podría ser de un run anterior si el objeto se reutilizaba),
       lo que hacía que la comparación de días en nextCard() fallara. */
    uint32_t prev_limit = m_session.session_limit;
    fsrs_session_start(&m_session, fsrs_now(), prev_limit);

    return true;
}

bool SrsService::save(const char *path)
{
    if (!m_deck) return false;

    std::string target = path ? std::string(path) : effectivePath();
    if (target.empty()) return false;

    if (!fsrs_save(m_deck, target.c_str()))
        return false;

    std::string snap = snapshot_path_for(target);
    std::string log  = log_path_for(target);

    return fsrs_sync_compact(&m_sync, m_deck, snap.c_str(), log.c_str(), fsrs_now());
}

/* ---- manipulación de cartas ---- */

bool SrsService::add(uint32_t entryId)
{
    if (!m_deck) return false;

    uint64_t now = fsrs_now();
    fsrs_card *card = fsrs_add_card(m_deck, entryId, now);
    if (!card) return false;

    FsrsEvent *ev = fsrs_sync_record_add(&m_sync, card, now);

    std::string log = log_path_for(effectivePath());
    (void)fsrs_sync_log_append(log.c_str(), ev);

    fsrs_rebuild_queues(m_deck, now);
    return true;
}

bool SrsService::remove(uint32_t entryId)
{
    if (!m_deck) return false;

    uint64_t now = fsrs_now();
    if (!fsrs_remove_card(m_deck, entryId)) return false;

    FsrsEvent *ev = fsrs_sync_record_remove(&m_sync, entryId, now);

    std::string log = log_path_for(effectivePath());
    (void)fsrs_sync_log_append(log.c_str(), ev);

    fsrs_rebuild_queues(m_deck, now);
    return true;
}

/*
 * reset() — BUG FIX respecto a la versión anterior en SrsLibraryViewModel.
 *
 * Antes, reset() manipulaba el card directamente y no registraba ningún
 * evento de sync ni compactaba. Esto significa que al recargar el perfil,
 * el reset se perdía.
 *
 * Ahora: remove + re-add = evento REMOVE + evento ADD en el log.
 */
bool SrsService::reset(uint32_t entryId)
{
    if (!m_deck) return false;

    uint64_t now = fsrs_now();

    /* Quitar la carta existente */
    if (!fsrs_remove_card(m_deck, entryId)) return false;
    {
        FsrsEvent *ev = fsrs_sync_record_remove(&m_sync, entryId, now);
        std::string log = log_path_for(effectivePath());
        (void)fsrs_sync_log_append(log.c_str(), ev);
    }

    /* Volver a añadir como nueva */
    fsrs_card *card = fsrs_add_card(m_deck, entryId, now);
    if (!card) return false;
    {
        FsrsEvent *ev = fsrs_sync_record_add(&m_sync, card, now);
        std::string log = log_path_for(effectivePath());
        (void)fsrs_sync_log_append(log.c_str(), ev);
    }

    fsrs_rebuild_queues(m_deck, now);
    return true;
}

/* ---- suspend / unsuspend / mark / snooze ---- */

bool SrsService::suspend(uint32_t entryId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();
    fsrs_suspend(m_deck, entryId);
    fsrs_rebuild_queues(m_deck, now);
    return compactSync(now);
}

bool SrsService::unsuspend(uint32_t entryId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();
    fsrs_unsuspend(m_deck, entryId, now);
    fsrs_rebuild_queues(m_deck, now);
    return compactSync(now);
}

bool SrsService::mark(uint32_t entryId, bool marked)
{
    if (!m_deck) return false;
    fsrs_mark(m_deck, entryId, marked);
    uint64_t now = fsrs_now();
    fsrs_rebuild_queues(m_deck, now);
    return compactSync(now);
}

bool SrsService::snooze(uint32_t entryId, uint32_t days)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();
    fsrs_snooze(m_deck, entryId, days, now);
    fsrs_rebuild_queues(m_deck, now);
    return compactSync(now);
}

/* ---- sesión / scheduling ---- */

void SrsService::startSession(uint32_t limit)
{
    fsrs_session_start(&m_session, fsrs_now(), limit);
}

/*
 * nextCard() — BUG FIX principal.
 *
 * 1. card_is_due usaba `now / FSRS_SEC_PER_DAY` (día UTC crudo) en vez de
 *    `fsrs_current_day(m_deck, now)` que respeta el day_offset configurado.
 *    Si el offset es != 0 (p.ej. 4 h → día empieza a las 04:00) la comparación
 *    era incorrecta durante las horas de transición.
 *
 * 2. La comparación de días para el reset de sesión es ahora correcta porque
 *    load() ya arranca la sesión con now, así que session_start siempre
 *    refleja el comienzo real de la sesión del día actual.
 */
fsrs_card* SrsService::nextCard()
{
    if (!m_deck) return nullptr;

    uint64_t now = fsrs_now();

    /* Reiniciar sesión si el día lógico cambió */
    uint64_t session_day = fsrs_current_day(m_deck, m_session.session_start);
    uint64_t current_day = fsrs_current_day(m_deck, now);
    if (session_day != current_day) {
        uint32_t limit = m_session.session_limit;
        fsrs_session_start(&m_session, now, limit);
    }

    fsrs_card *card = fsrs_next_for_session(m_deck, &m_session, now);
    if (!card) return nullptr;

    /* BUG FIX: usar fsrs_current_day en vez de división cruda */
    if (card->state == FSRS_STATE_SUSPENDED) return nullptr;

    uint64_t card_day = (uint64_t)card->due_day;
    if (card_day > current_day) return nullptr;   // no está debida aún hoy

    return card;
}

void SrsService::answer(uint32_t entryId, fsrs_rating rating)
{
    if (!m_deck) return;

    uint64_t now = fsrs_now();
    fsrs_card *card = getCard(entryId);
    if (!card) return;

    uint8_t prev_state = card->state;

    fsrs_answer(m_deck, card, rating, now);

    /* Actualizar contadores de sesión */
    m_session.cards_done += 1;
    if (prev_state == FSRS_STATE_NEW)
        m_session.new_done += 1;
    else if (prev_state == FSRS_STATE_LEARNING || prev_state == FSRS_STATE_RELEARNING)
        m_session.learn_done += 1;
    else if (prev_state == FSRS_STATE_REVIEW)
        m_session.review_done += 1;

    fsrs_rebuild_queues(m_deck, now);

    FsrsEvent *ev = fsrs_sync_record_answer(&m_sync, card, rating, now);
    std::string log = log_path_for(effectivePath());
    (void)fsrs_sync_log_append(log.c_str(), ev);
}

bool SrsService::contains(uint32_t entryId) const
{
    return fsrs_get_card(m_deck, entryId) != nullptr;
}

/* ---- estadísticas ---- */

uint32_t SrsService::totalCount() const
{
    if (!m_deck) return 0;
    return fsrs_deck_count(m_deck);
}

uint32_t SrsService::dueCount() const
{
    if (!m_deck) return 0;
    fsrs_stats st;
    fsrs_compute_stats(m_deck, fsrs_now(), &st);
    return st.due_now;
}

uint32_t SrsService::learningCount() const
{
    if (!m_deck) return 0;
    fsrs_stats st;
    fsrs_compute_stats(m_deck, fsrs_now(), &st);
    return st.learning_count + st.relearning_count;
}

uint32_t SrsService::newCount() const
{
    if (!m_deck) return 0;
    fsrs_stats st;
    fsrs_compute_stats(m_deck, fsrs_now(), &st);
    return st.new_count;
}

uint32_t SrsService::lapsedCount() const
{
    if (!m_deck) return 0;
    fsrs_stats st;
    fsrs_compute_stats(m_deck, fsrs_now(), &st);
    return st.cards_with_lapses;
}

/*
 * reviewTodayCount() — BUG FIX.
 *
 * Antes se usaba st.due_today (cartas *pendientes* hoy), no las ya *respondidas*.
 * Ahora devolvemos m_session.cards_done, que es el conteo real de respuestas
 * dadas en la sesión del día actual (se resetea con la sesión al cambiar de día).
 */
uint32_t SrsService::reviewTodayCount() const
{
    return m_session.cards_done;
}

uint64_t SrsService::waitSeconds() const
{
    if (!m_deck) return 0;
    return fsrs_wait_seconds(m_deck, fsrs_now());
}

/* ---- preview intervals ---- */

std::string SrsService::predictInterval(uint32_t entryId, fsrs_rating rating)
{
    fsrs_card *card = getCard(entryId);
    if (!card) return {};

    uint64_t now = fsrs_now();
    uint64_t out[4] = {0, 0, 0, 0};
    fsrs_preview(m_deck, card, now, out);

    int idx = static_cast<int>(rating) - 1;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;

    uint64_t due_ts = out[idx];
    uint64_t delta  = (due_ts > now) ? (due_ts - now) : 0;

    char buf[64] = {0};
    fsrs_format_interval(delta, buf, sizeof(buf));
    return std::string(buf);
}

/*
 * dueDateText() — NUEVO.
 *
 * Devuelve el tiempo que falta hasta la fecha de vencimiento *real* de la
 * carta (card->due_ts), en vez de predecir el intervalo de una respuesta
 * hipotética. Esto es lo que debe mostrarse en la columna "Due" de la librería.
 */
std::string SrsService::dueDateText(uint32_t entryId) const
{
    fsrs_card *card = getCard(entryId);
    if (!card) return {};

    uint64_t now = fsrs_now();

    /* Cartas nuevas: mostrar "New" */
    if (card->state == FSRS_STATE_NEW) return "New";

    /* Cartas suspendidas */
    if (card->state == FSRS_STATE_SUSPENDED) return "Suspended";

    uint64_t due = card->due_ts;
    if (due <= now) return "Now";   // ya debida

    uint64_t delta = due - now;
    char buf[64] = {0};
    fsrs_format_interval(delta, buf, sizeof(buf));
    return std::string(buf);
}