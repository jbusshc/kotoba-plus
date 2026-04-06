#include "SrsService.h"
#include "SrsHistoryLog.h"
#include "CardType.h"
#include "../app/Configuration.h"
#include "../../core/include/fsrs.h"
#include <string>
#include <cstring>
#include <QThreadPool>

/* ─── path helpers ─────────────────────────────────────────────────────────── */

static inline std::string snapshot_path_for(const std::string &base) { return base + ".sync.snap"; }
static inline std::string log_path_for     (const std::string &base) { return base + ".sync.log";  }

/* ─── ctor / dtor ──────────────────────────────────────────────────────────── */

SrsService::SrsService() : m_deck(nullptr), m_config(nullptr) {}

void SrsService::initialize(uint32_t dictSize, Configuration *config)
{
    m_config   = config;
    m_dictSize = dictSize;

    fsrs_params params = fsrs_default_params();

    // El id_map y el bitmap del deck C son arrays directos indexados por ID.
    // Hay que pre-dimensionarlos para cubrir el rango completo:
    //   Recognition: [0,             dictSize)
    //   Recall:      [RECALL_OFFSET, RECALL_OFFSET + dictSize)
    // Pasamos el ID máximo posible + 1 como hint para evitar reallocs gigantes.
    const uint32_t idSpaceHint = CardTypeHelper::RECALL_OFFSET + dictSize;
    m_deck = fsrs_deck_create(idSpaceHint, &params);

    if (m_config) fsrs_seed(m_config->deviceId);

    uint64_t device_id = m_config ? m_config->deviceId : 0;
    fsrs_sync_init(&m_sync, device_id);
    fsrs_session_start(&m_session, fsrs_now(), 0);
}

SrsService::~SrsService()
{
    fsrs_sync_free(&m_sync);
    if (m_deck) fsrs_deck_free(m_deck);
}

/* ─── helpers privados ──────────────────────────────────────────────────────── */

fsrs_card* SrsService::getCard(uint32_t fsrsId) const
{
    if (!m_deck) return nullptr;
    return fsrs_get_card(m_deck, fsrsId);
}

std::string SrsService::effectivePath() const
{
    if (!m_profilePath.empty()) return m_profilePath;
    if (m_config && !m_config->srsPath.isEmpty())
        return m_config->srsPath.toStdString();
    return {};
}

bool SrsService::compactSync(uint64_t now)
{
    std::string base = effectivePath();
    if (base.empty()) return false;
    return fsrs_sync_compact(&m_sync, m_deck,
                             snapshot_path_for(base).c_str(),
                             log_path_for(base).c_str(), now);
}

/* ─── operaciones de bajo nivel (fsrsId completo) ──────────────────────────── */

bool SrsService::_add(uint32_t fsrsId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();
    fsrs_card *card = fsrs_add_card(m_deck, fsrsId, now);
    if (!card) return false;

    if (m_history) m_history->recordAdd(card, now);

    const std::string path = effectivePath();
    if (!path.empty()) {
        FsrsEvent *ev = fsrs_sync_record_add(&m_sync, card, now);
        (void)fsrs_sync_log_append(log_path_for(path).c_str(), ev);

        // Flush inmediato del .srs para que el add sobreviva a un crash.
        // Es barato porque solo escribe la carta nueva al final del archivo.
        (void)fsrs_save(m_deck, path.c_str());
    }
    return true;
}

bool SrsService::_remove(uint32_t fsrsId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();

    const fsrs_card *before = fsrs_get_card(m_deck, fsrsId);
    fsrs_card before_copy = before ? *before : fsrs_card{};
    const bool had_card   = (before != nullptr);

    if (!fsrs_remove_card(m_deck, fsrsId)) return false;

    if (m_history && had_card)
        m_history->recordRemove(fsrsId, &before_copy, now);

    const std::string path = effectivePath();
    if (!path.empty()) {
        FsrsEvent *ev = fsrs_sync_record_remove(&m_sync, fsrsId, now);
        (void)fsrs_sync_log_append(log_path_for(path).c_str(), ev);
        (void)fsrs_save(m_deck, path.c_str());
    }
    return true;
}

bool SrsService::_contains(uint32_t fsrsId) const
{
    return fsrs_get_card(m_deck, fsrsId) != nullptr;
}

bool SrsService::_reset(uint32_t fsrsId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();

    if (!fsrs_remove_card(m_deck, fsrsId)) return false;
    {
        FsrsEvent *ev = fsrs_sync_record_remove(&m_sync, fsrsId, now);
        (void)fsrs_sync_log_append(log_path_for(effectivePath()).c_str(), ev);
    }

    fsrs_card *card = fsrs_add_card(m_deck, fsrsId, now);
    if (!card) return false;
    {
        FsrsEvent *ev = fsrs_sync_record_add(&m_sync, card, now);
        (void)fsrs_sync_log_append(log_path_for(effectivePath()).c_str(), ev);
    }
    return true;
}

bool SrsService::_suspend(uint32_t fsrsId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();

    const fsrs_card *sc = fsrs_get_card(m_deck, fsrsId);
    fsrs_card before = sc ? *sc : fsrs_card{};
    const bool had_sc = (sc != nullptr);

    fsrs_suspend(m_deck, fsrsId);
    fsrs_rebuild_queues(m_deck, now);

    if (m_history && had_sc) m_history->recordSuspend(&before, now);
    return compactSync(now);
}

bool SrsService::_unsuspend(uint32_t fsrsId)
{
    if (!m_deck) return false;
    uint64_t now = fsrs_now();

    fsrs_unsuspend(m_deck, fsrsId, now);
    fsrs_rebuild_queues(m_deck, now);

    const fsrs_card *after = fsrs_get_card(m_deck, fsrsId);
    if (m_history) m_history->recordUnsuspend(after, now);
    return compactSync(now);
}

/* ─── API pública ───────────────────────────────────────────────────────────── */

bool SrsService::add(uint32_t entryId, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    uint32_t fsrsId = CardTypeHelper::toFsrsId(entryId, type);
    bool ok = _add(fsrsId);

    // Auto-add Recall cuando se agrega Recognition (si la opción está activa)
    if (ok && type == CardType::Recognition && m_autoAddRecall) {
        uint32_t recallId = CardTypeHelper::toFsrsId(entryId, CardType::Recall);
        _add(recallId);   // falla silenciosamente si ya existe
    }

    if (ok) fsrs_rebuild_queues(m_deck, fsrs_now());
    return ok;
}

bool SrsService::addBoth(uint32_t entryId)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    uint64_t now = fsrs_now();
    bool ok1 = _add(CardTypeHelper::toFsrsId(entryId, CardType::Recognition));
    bool ok2 = _add(CardTypeHelper::toFsrsId(entryId, CardType::Recall));
    if (ok1 || ok2) fsrs_rebuild_queues(m_deck, now);
    return ok1 || ok2;
}

bool SrsService::remove(uint32_t entryId, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    bool ok = _remove(CardTypeHelper::toFsrsId(entryId, type));
    if (ok) fsrs_rebuild_queues(m_deck, fsrs_now());
    return ok;
}

void SrsService::removeBoth(uint32_t entryId)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    uint64_t now = fsrs_now();
    _remove(CardTypeHelper::toFsrsId(entryId, CardType::Recognition));
    _remove(CardTypeHelper::toFsrsId(entryId, CardType::Recall));
    fsrs_rebuild_queues(m_deck, now);
}

bool SrsService::contains(uint32_t entryId, CardType type) const
{
    return _contains(CardTypeHelper::toFsrsId(entryId, type));
}

bool SrsService::reset(uint32_t entryId, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    bool ok = _reset(CardTypeHelper::toFsrsId(entryId, type));
    if (ok) fsrs_rebuild_queues(m_deck, fsrs_now());
    return ok;
}

bool SrsService::suspend(uint32_t entryId, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    return _suspend(CardTypeHelper::toFsrsId(entryId, type));
}

bool SrsService::unsuspend(uint32_t entryId, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    return _unsuspend(CardTypeHelper::toFsrsId(entryId, type));
}

bool SrsService::mark(uint32_t entryId, bool marked, CardType type)
{
    if (!m_deck) return false;
    uint32_t fsrsId = CardTypeHelper::toFsrsId(entryId, type);
    fsrs_mark(m_deck, fsrsId, marked);
    uint64_t now = fsrs_now();
    fsrs_rebuild_queues(m_deck, now);
    return compactSync(now);
}

bool SrsService::snooze(uint32_t entryId, uint32_t days, CardType type)
{
    if (!m_deck) return false;
    uint32_t fsrsId = CardTypeHelper::toFsrsId(entryId, type);
    uint64_t now = fsrs_now();
    fsrs_snooze(m_deck, fsrsId, days, now);
    fsrs_rebuild_queues(m_deck, now);
    return compactSync(now);
}

/* ─── sesión / scheduling ───────────────────────────────────────────────────── */

void SrsService::startSession(uint32_t limit)
{
    fsrs_session_start(&m_session, fsrs_now(), limit);
}

fsrs_card* SrsService::nextCard()
{
    if (!m_deck) return nullptr;

    uint64_t now = fsrs_now();

    // Reset de sesión si cambió el día lógico
    uint64_t session_day = fsrs_current_day(m_deck, m_session.session_start);
    uint64_t current_day = fsrs_current_day(m_deck, now);
    if (session_day != current_day) {
        uint32_t limit = m_session.session_limit;
        fsrs_session_start(&m_session, now, limit);
    }

    fsrs_card *card = fsrs_next_for_session(m_deck, &m_session, now);
    if (!card) return nullptr;

    if (card->state == FSRS_STATE_SUSPENDED) return nullptr;

    if ((uint64_t)card->due_day > current_day) return nullptr;

    return card;
}

void SrsService::answer(uint32_t fsrsId, fsrs_rating rating)
{
    if (!m_deck) return;

    uint64_t now   = fsrs_now();
    fsrs_card *card = getCard(fsrsId);
    if (!card) return;

    m_undoStack = UndoEntry{
        .card        = *card,
        .fsrsId      = fsrsId,
        .cards_done  = m_session.cards_done,
        .new_done    = m_session.new_done,
        .learn_done  = m_session.learn_done,
        .review_done = m_session.review_done
    };

    uint8_t prev_state = card->state;
    fsrs_answer(m_deck, card, rating, now);

    m_session.cards_done++;
    if      (prev_state == FSRS_STATE_NEW)                               m_session.new_done++;
    else if (prev_state == FSRS_STATE_LEARNING ||
             prev_state == FSRS_STATE_RELEARNING)                        m_session.learn_done++;
    else if (prev_state == FSRS_STATE_REVIEW)                            m_session.review_done++;

    fsrs_rebuild_queues(m_deck, now);

    if (m_history)
        m_history->recordAnswer(fsrs_get_card(m_deck, fsrsId), rating, now);

    FsrsEvent *ev = fsrs_sync_record_answer(&m_sync, card, rating, now);
    (void)fsrs_sync_log_append(log_path_for(effectivePath()).c_str(), ev);
}

bool SrsService::undoLastAnswer()
{
    if (!m_deck || !m_undoStack.has_value()) return false;

    const UndoEntry &undo = *m_undoStack;
    fsrs_card *card = fsrs_get_card(m_deck, undo.fsrsId);
    if (!card) return false;

    *card = undo.card;
    card->heap_pos_due   = -1;
    card->heap_pos_learn = -1;

    m_session.cards_done  = undo.cards_done;
    m_session.new_done    = undo.new_done;
    m_session.learn_done  = undo.learn_done;
    m_session.review_done = undo.review_done;

    FsrsEvent *ev = fsrs_sync_record_undo(&m_sync, undo.fsrsId, fsrs_now());
    if (ev) (void)fsrs_sync_log_append(log_path_for(effectivePath()).c_str(), ev);

    fsrs_rebuild_queues(m_deck, fsrs_now());

    if (m_history)
        m_history->recordUndo(undo.fsrsId,
                              fsrs_get_card(m_deck, undo.fsrsId),
                              fsrs_now());

    m_undoStack.reset();
    return true;
}

/* ─── estadísticas ──────────────────────────────────────────────────────────── */

uint32_t SrsService::totalCount() const
{
    return m_deck ? fsrs_deck_count(m_deck) : 0;
}

uint32_t SrsService::dueCount() const
{
    if (!m_deck) return 0;
    uint64_t now   = fsrs_now();
    uint64_t today = fsrs_current_day(m_deck, now);
    uint32_t count = 0;
    uint32_t total = fsrs_deck_count(m_deck);
    for (uint32_t i = 0; i < total; ++i) {
        const fsrs_card *c = fsrs_deck_card(m_deck, i);
        if (!c || c->state == FSRS_STATE_SUSPENDED) continue;
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
    return m_deck ? fsrs_wait_seconds(m_deck, fsrs_now()) : 0;
}

/* ─── preview / texto ───────────────────────────────────────────────────────── */

std::string SrsService::predictInterval(uint32_t fsrsId, fsrs_rating rating)
{
    fsrs_card *card = getCard(fsrsId);
    if (!card) return {};

    uint64_t now = fsrs_now();
    uint64_t out[4] = {0, 0, 0, 0};
    fsrs_preview(m_deck, card, now, out);

    int idx = static_cast<int>(rating) - 1;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;

    uint64_t delta = (out[idx] > now) ? (out[idx] - now) : 0;
    char buf[64] = {0};
    fsrs_format_interval(delta, buf, sizeof(buf));
    return std::string(buf);
}

std::string SrsService::dueDateText(uint32_t fsrsId) const
{
    fsrs_card *card = getCard(fsrsId);
    if (!card) return {};

    uint64_t now = fsrs_now();
    if (card->state == FSRS_STATE_NEW)       return "New";
    if (card->state == FSRS_STATE_SUSPENDED) return "Suspended";

    uint64_t due = card->due_ts;
    if (due <= now) return "Now";

    char buf[64] = {0};
    fsrs_format_interval(due - now, buf, sizeof(buf));
    return std::string(buf);
}

std::string SrsService::stateText(uint32_t fsrsId) const
{
    fsrs_card *card = getCard(fsrsId);
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

/* ─── config ────────────────────────────────────────────────────────────────── */

void SrsService::updateConfig(const Configuration *config)
{
    if (!config) return;
    m_config = config;

    setAutoAddRecall(config->autoAddRecall);

    update_srs_config(m_deck,
        nullptr,
        config->desiredRetention, config->maximumInterval, config->leechThreshold,
        config->dayOffset, config->newCardsPerDay, config->reviewsPerDay,
        nullptr, 0,
        nullptr, 0,
        0, 0, config->enableFuzz,
        static_cast<fsrs_order_mode>(config->orderMode)
    );
}

/* ─── persistencia ──────────────────────────────────────────────────────────── */

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

    if (!fsrs_save(m_deck, target.c_str())) return false;

    return fsrs_sync_compact(&m_sync, m_deck,
                             snapshot_path_for(target).c_str(),
                             log_path_for(target).c_str(),
                             fsrs_now());
}

void SrsService::saveAsync(const char *path)
{
    std::string target = path ? std::string(path) : effectivePath();
    if (target.empty()) return;

    QThreadPool::globalInstance()->start([this, target]() {
        std::lock_guard<std::mutex> lock(m_deckMutex);
        if (!m_deck) return;
        (void)fsrs_save(m_deck, target.c_str());
        (void)fsrs_sync_compact(&m_sync, m_deck,
                                snapshot_path_for(target).c_str(),
                                log_path_for(target).c_str(),
                                fsrs_now());
    });
}