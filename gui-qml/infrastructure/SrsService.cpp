#include "SrsService.h"
#include "SrsHistoryLog.h"
#include "../app/Configuration.h"
#include "../../core/include/fsrs.h"
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

SrsService::SrsService()
    : m_deck(nullptr), m_config(nullptr)
{
}

void SrsService::initialize(uint32_t dictSize, Configuration *config)
{
    m_config = config;
    m_dictSize = dictSize;
    fsrs_params params = fsrs_default_params();
    m_deck = fsrs_deck_create(dictSize, &params);

    if (m_config)
        fsrs_seed(m_config->deviceId);

    uint64_t device_id = m_config ? m_config->deviceId : 0;
    fsrs_sync_init(&m_sync, device_id);

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

    m_history = std::make_unique<SrsHistoryLog>(m_profilePath);

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

    if (m_history)
        m_history->recordAdd(card, now);

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

    const fsrs_card *before = fsrs_get_card(m_deck, entryId);
    fsrs_card before_copy = before ? *before : fsrs_card{};
    const bool had_card = (before != nullptr);

    if (!fsrs_remove_card(m_deck, entryId)) return false;

    if (m_history && had_card)
        m_history->recordRemove(entryId, &before_copy, now);

    FsrsEvent *ev = fsrs_sync_record_remove(&m_sync, entryId, now);
    std::string log = log_path_for(effectivePath());
    (void)fsrs_sync_log_append(log.c_str(), ev);

    fsrs_rebuild_queues(m_deck, now);
    return true;
}

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

    const fsrs_card *sc = fsrs_get_card(m_deck, entryId);
    fsrs_card before = sc ? *sc : fsrs_card{};
    const bool had_sc = (sc != nullptr);

    fsrs_suspend(m_deck, entryId);
    fsrs_rebuild_queues(m_deck, now);

    if (m_history && had_sc)
        m_history->recordSuspend(&before, now);

    return compactSync(now);
}

bool SrsService::unsuspend(uint32_t entryId)
{
    if (!m_deck) return false;

    uint64_t now = fsrs_now();

    fsrs_unsuspend(m_deck, entryId, now);
    fsrs_rebuild_queues(m_deck, now);

    const fsrs_card *after = fsrs_get_card(m_deck, entryId);
    if (m_history)
        m_history->recordUnsuspend(after, now);

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

fsrs_card* SrsService::nextCard()
{
    if (!m_deck) return nullptr;

    uint64_t now = fsrs_now();

    uint64_t session_day = fsrs_current_day(m_deck, m_session.session_start);
    uint64_t current_day = fsrs_current_day(m_deck, now);
    if (session_day != current_day) {
        uint32_t limit = m_session.session_limit;
        fsrs_session_start(&m_session, now, limit);
    }

    fsrs_card *card = fsrs_next_for_session(m_deck, &m_session, now);
    if (!card) return nullptr;

    if (card->state == FSRS_STATE_SUSPENDED) return nullptr;

    uint64_t card_day = (uint64_t)card->due_day;
    if (card_day > current_day) return nullptr;

    return card;
}

bool SrsService::undoLastAnswer()
{
    if (!m_deck || !m_undoStack.has_value()) return false;

    const UndoEntry &undo = *m_undoStack;
    fsrs_card *card = fsrs_get_card(m_deck, undo.entryId);
    if (!card) return false;

    *card = undo.card;
    card->heap_pos_due   = -1;
    card->heap_pos_learn = -1;

    m_session.cards_done  = undo.cards_done;
    m_session.new_done    = undo.new_done;
    m_session.learn_done  = undo.learn_done;
    m_session.review_done = undo.review_done;

    FsrsEvent *ev = fsrs_sync_record_undo(&m_sync, undo.entryId, fsrs_now());
    if (ev) {
        std::string log = log_path_for(effectivePath());
        (void)fsrs_sync_log_append(log.c_str(), ev);
    }

    fsrs_rebuild_queues(m_deck, fsrs_now());

    if (m_history)
        m_history->recordUndo(undo.entryId,
                              fsrs_get_card(m_deck, undo.entryId),
                              fsrs_now());

    m_undoStack.reset();
    return true;
}

void SrsService::answer(uint32_t entryId, fsrs_rating rating)
{
    if (!m_deck) return;

    uint64_t now = fsrs_now();
    fsrs_card *card = getCard(entryId);
    if (!card) return;

    m_undoStack = UndoEntry{
        .card        = *card,
        .entryId     = entryId,
        .cards_done  = m_session.cards_done,
        .new_done    = m_session.new_done,
        .learn_done  = m_session.learn_done,
        .review_done = m_session.review_done
    };

    uint8_t prev_state = card->state;

    fsrs_answer(m_deck, card, rating, now);

    m_session.cards_done += 1;
    if (prev_state == FSRS_STATE_NEW)
        m_session.new_done += 1;
    else if (prev_state == FSRS_STATE_LEARNING || prev_state == FSRS_STATE_RELEARNING)
        m_session.learn_done += 1;
    else if (prev_state == FSRS_STATE_REVIEW)
        m_session.review_done += 1;

    fsrs_rebuild_queues(m_deck, now);

    if (m_history)
        m_history->recordAnswer(fsrs_get_card(m_deck, entryId), rating, now);

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
    /*
     * due_now from fsrs_compute_stats includes ALL cards whose due_day <= today,
     * including suspended ones. We compute manually to exclude them.
     */
    uint64_t now = fsrs_now();
    uint64_t today = fsrs_current_day(m_deck, now);
    uint32_t count = 0;
    uint32_t total = fsrs_deck_count(m_deck);
    for (uint32_t i = 0; i < total; ++i) {
        const fsrs_card *c = fsrs_deck_card(m_deck, i);
        if (!c) continue;
        if (c->state == FSRS_STATE_SUSPENDED) continue;
        if ((uint64_t)c->due_day <= today) ++count;
    }
    return count;
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

std::string SrsService::dueDateText(uint32_t entryId) const
{
    fsrs_card *card = getCard(entryId);
    if (!card) return {};

    uint64_t now = fsrs_now();

    if (card->state == FSRS_STATE_NEW)       return "New";
    if (card->state == FSRS_STATE_SUSPENDED) return "Suspended";

    uint64_t due = card->due_ts;
    if (due <= now) return "Now";

    uint64_t delta = due - now;
    char buf[64] = {0};
    fsrs_format_interval(delta, buf, sizeof(buf));
    return std::string(buf);
}

std::string SrsService::stateText(uint32_t entryId) const
{
    fsrs_card *card = getCard(entryId);
    if (!card) return {};

    switch (card->state) {
        case FSRS_STATE_NEW:        return "New";
        case FSRS_STATE_LEARNING:   return "Learning";
        case FSRS_STATE_RELEARNING: return "Relearning";
        case FSRS_STATE_REVIEW:     return "Review";
        case FSRS_STATE_SUSPENDED:  return "Suspended";
        default:                    return "Unknown";
    }
}

void SrsService::updateConfig(const Configuration* config)
{
    if (!config) return;

    m_config = config;

    // srs 
    update_srs_config(m_deck,
        nullptr, // w no se actualiza dinámicamente 
        config->desiredRetention, config->maximumInterval, config->leechThreshold,
        config->dayOffset, config->newCardsPerDay, config->reviewsPerDay,
        nullptr, 0, // learning steps no se actualizan dinámicamente
        nullptr, 0, // relearning steps no se actualizan dinámicamente
        0, 0, config->enableFuzz,
        static_cast<fsrs_order_mode>(config->orderMode)
    );

}