#include "SrsService.h"
#include "../app/Configuration.h"

#include <string>

static inline std::string snapshot_path_for(const char* base)
{
    return std::string(base) + ".sync.snap";
}

static inline std::string log_path_for(const char* base)
{
    return std::string(base) + ".sync.log";
}

SrsService::SrsService(uint32_t dictSize, Configuration *config)
    : m_dictSize(dictSize), m_config(config)
{
    fsrs_params params = fsrs_default_params();

    /* crear deck vacío */
    m_deck = fsrs_deck_create(dictSize, &params);

    /* inicializar RNG con deviceId (si existe) */
    if (m_config)
        fsrs_seed(m_config->deviceId);

    /* init sync */
    uint64_t device_id = m_config ? m_config->deviceId : 0;
    fsrs_sync_init(&m_sync, device_id);

    /* sesión por defecto */
    fsrs_session_start(&m_session, fsrs_now(), 0);
}

SrsService::~SrsService()
{
    fsrs_sync_free(&m_sync);

    if (m_deck)
        fsrs_deck_free(m_deck);
}

/* ---- helpers ---- */
fsrs_card* SrsService::getCard(uint32_t entryId) const
{
    if (!m_deck) return nullptr;
    return fsrs_get_card(m_deck, entryId);
}

/* ---- persistence ---- */
bool SrsService::load(const char *path)
{
    if (!path) return false;

    fsrs_params params = fsrs_default_params();

    fsrs_deck *deck = fsrs_load(path, &params);
    if (!deck)
        return false;

    m_deck = deck;
    m_profilePath = path;

    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    /* cargar snapshot (si existe) */
    (void)fsrs_sync_snapshot_load(m_deck, snap.c_str(), fsrs_now());

    /* cargar log de eventos y proyectar */
    (void)fsrs_sync_log_load(&m_sync, log.c_str());

    fsrs_sync_rebuild(&m_sync, m_deck, fsrs_now());

    /* reconstruir colas (seguridad) */
    fsrs_rebuild_queues(m_deck, fsrs_now());

    return true;
}

bool SrsService::save(const char *path)
{
    if (!m_deck || !path) return false;

    if (!fsrs_save(m_deck, path))
        return false;

    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    return fsrs_sync_compact(&m_sync, m_deck, snap.c_str(), log.c_str(), fsrs_now());
}

/* ---- manipulation ---- */
bool SrsService::add(uint32_t entryId)
{
    if (!m_deck) return false;

    uint64_t now = fsrs_now();

    fsrs_card *card = fsrs_add_card(m_deck, entryId, now);
    if (!card)
        return false;

    /* registrar evento local */
    FsrsEvent *ev = fsrs_sync_record_add(&m_sync, card, now);

    std::string base = m_profilePath.empty() && m_config ? m_config->srsPath.toStdString() : m_profilePath;
    std::string log = log_path_for(base.c_str());

    (void)fsrs_sync_log_append(log.c_str(), ev);

    /* rebuild queues (opcional) */
    fsrs_rebuild_queues(m_deck, now);

    return true;
}

bool SrsService::remove(uint32_t entryId)
{
    if (!m_deck) return false;

    uint64_t now = fsrs_now();

    if (!fsrs_remove_card(m_deck, entryId))
        return false;

    FsrsEvent *ev = fsrs_sync_record_remove(&m_sync, entryId, now);

    std::string base = m_profilePath.empty() && m_config ? m_config->srsPath.toStdString() : m_profilePath;
    std::string log = log_path_for(base.c_str());

    (void)fsrs_sync_log_append(log.c_str(), ev);

    fsrs_rebuild_queues(m_deck, now);

    return true;
}

bool SrsService::contains(uint32_t entryId) const
{
    return getCard(entryId) != nullptr;
}

/* ---- session / scheduling ---- */
void SrsService::startSession(uint32_t limit)
{
    fsrs_session_start(&m_session, fsrs_now(), limit);
}

inline bool card_is_due(const fsrs_card* card, uint64_t now)
{
    if (!card) return false;
    uint64_t today = now / FSRS_SEC_PER_DAY;
    return !(card->state == FSRS_STATE_SUSPENDED) && (card->due_day <= today);
}

fsrs_card* SrsService::nextCard()
{
    if (!m_deck) return nullptr;

    uint64_t now = fsrs_now();

    // reiniciar sesión si cambió el día lógico
    uint64_t session_day = fsrs_current_day(m_deck, m_session.session_start);
    uint64_t current_day = fsrs_current_day(m_deck, now);
    if (session_day != current_day) {
        uint32_t limit = m_session.session_limit;
        fsrs_session_start(&m_session, now, limit);
    }

    // obtener la siguiente carta de la sesión
    fsrs_card* card = fsrs_next_for_session(m_deck, &m_session, now);

    // verificar si realmente está debida hoy
    if (card && !card_is_due(card, now))
        return nullptr;

    return card;
}

void SrsService::answer(uint32_t entryId, fsrs_rating rating)
{
    if (!m_deck) return;

    uint64_t now = fsrs_now();

    fsrs_card *card = getCard(entryId);
    if (!card) return;

    // Guardar el estado previo para contabilizar correctamente la respuesta
    uint8_t prev_state = card->state;

    // Aplicar la respuesta (actualiza estabilidad, due, etc.)
    fsrs_answer(m_deck, card, rating, now);

    // Actualizar contadores de sesión manualmente para que la sesión avance.
    // Nota: fsrs_next_for_session usa estos contadores para respetar limits/mix.
    m_session.cards_done += 1;
    if (prev_state == FSRS_STATE_NEW) {
        m_session.new_done += 1;
    } else if (prev_state == FSRS_STATE_LEARNING || prev_state == FSRS_STATE_RELEARNING) {
        m_session.learn_done += 1;
    } else if (prev_state == FSRS_STATE_REVIEW) {
        m_session.review_done += 1;
    }

    // Reconstruir colas para que los cambios de 'due' / estado se reflejen inmediatamente.
    fsrs_rebuild_queues(m_deck, now);

    // Registrar evento de sync (snapshot resultante)
    FsrsEvent *ev = fsrs_sync_record_answer(&m_sync, card, rating, now);

    std::string base = m_profilePath.empty() && m_config ? m_config->srsPath.toStdString() : m_profilePath;
    std::string log = log_path_for(base.c_str());

    (void)fsrs_sync_log_append(log.c_str(), ev);
}

/* ---- stats ---- */
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

uint64_t SrsService::waitSeconds() const
{
    if (!m_deck) return 0;
    return fsrs_wait_seconds(m_deck, fsrs_now());
}

/* ---- preview intervals ---- */
std::string SrsService::predictInterval(uint32_t entryId, fsrs_rating rating)
{
    fsrs_card *card = getCard(entryId);
    if (!card) return std::string();

    uint64_t now = fsrs_now();
    uint64_t out[4] = {0,0,0,0};

    fsrs_preview(m_deck, card, now, out);

    int idx = (int)rating - 1;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;

    uint64_t due_ts = out[idx];

    // Si fsrs_preview devolvió 0 (no previsto), o due <= now, mostrar "0s" o "now"
    uint64_t delta = 0;
    if (due_ts > now) delta = due_ts - now;
    else delta = 0;

    char buf[64] = {0};
    fsrs_format_interval(delta, buf, sizeof(buf));

    return std::string(buf);
}