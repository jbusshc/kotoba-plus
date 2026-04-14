#include "SrsService.h"
#include "SrsHistoryLog.h"
#include "../app/Configuration.h"

#include <cstring>
#include <QThreadPool>
#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

// ─────────────────────────────────────────────────────────────────────────────
// rutas
// ─────────────────────────────────────────────────────────────────────────────

static const char *typeSuffix(CardType type)
{
    return (type == CardType::Recall) ? ".recall" : ".recog";
}

std::string SrsService::srsPath(CardType type) const
{
    return m_basePath + typeSuffix(type) + ".srs";
}
std::string SrsService::snapPath(CardType type) const
{
    return m_basePath + typeSuffix(type) + ".sync.snap";
}
std::string SrsService::logPath(CardType type) const
{
    return m_basePath + typeSuffix(type) + ".sync.log";
}
std::string SrsService::sessionPath() const
{
    return m_basePath + ".session.json";
}
std::string SrsService::customDecksPath() const
{
    return m_basePath + ".custom_decks.json";
}

// ─────────────────────────────────────────────────────────────────────────────
// session persistence — guarda/restaura cards_done del día
// ─────────────────────────────────────────────────────────────────────────────

void SrsService::saveSession() const
{
    if (m_basePath.empty()) return;

    // Guardamos el día lógico (día FSRS, no calendario) y los contadores.
    // Al cargar, si el día cambió, reseteamos a 0.
    uint64_t now = fsrs_now();
    uint64_t today = m_recognition
        ? fsrs_current_day(m_recognition, now)
        : (now / 86400ULL);

    QJsonObject obj;
    obj["day"]              = static_cast<qint64>(today);
    obj["recog_cards_done"] = static_cast<int>(m_recogSession.cards_done);
    obj["recall_cards_done"]= static_cast<int>(m_recallSession.cards_done);

    QFile f(QString::fromStdString(sessionPath()));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void SrsService::loadSession()
{
    if (m_basePath.empty()) return;

    QFile f(QString::fromStdString(sessionPath()));
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) return;

    QJsonObject obj = doc.object();

    uint64_t now   = fsrs_now();
    uint64_t today = m_recognition
        ? fsrs_current_day(m_recognition, now)
        : (now / 86400ULL);

    uint64_t savedDay = static_cast<uint64_t>(obj["day"].toInteger(0));

    // Solo restauramos si es el mismo día lógico
    if (savedDay == today) {
        m_recogSession.cards_done  = static_cast<uint32_t>(obj["recog_cards_done"].toInt(0));
        m_recallSession.cards_done = static_cast<uint32_t>(obj["recall_cards_done"].toInt(0));
    }
    // Si el día cambió, los contadores quedan en 0 (ya inicializados por fsrs_session_start)
}

// ─────────────────────────────────────────────────────────────────────────────
// helpers de acceso por tipo
// ─────────────────────────────────────────────────────────────────────────────

fsrs_deck *SrsService::deckFor(CardType type) const
{
    return (type == CardType::Recall) ? m_recall : m_recognition;
}
FsrsSync &SrsService::syncFor(CardType type)
{
    return (type == CardType::Recall) ? m_recallSync : m_recogSync;
}
SrsHistoryLog *SrsService::historyFor(CardType type) const
{
    return (type == CardType::Recall) ? m_recallHistory.get() : m_recogHistory.get();
}
fsrs_session &SrsService::sessionFor(CardType type)
{
    return (type == CardType::Recall) ? m_recallSession : m_recogSession;
}
fsrs_card *SrsService::getCard(uint32_t entSeq, CardType type) const
{
    fsrs_deck *d = deckFor(type);
    if (!d) return nullptr;
    return fsrs_get_card(d, entSeq);
}

// ─────────────────────────────────────────────────────────────────────────────
// ctor / dtor
// ─────────────────────────────────────────────────────────────────────────────

SrsService::SrsService() {}

SrsService::~SrsService()
{
    fsrs_sync_free(&m_recogSync);
    fsrs_sync_free(&m_recallSync);
    if (m_recognition) fsrs_deck_free(m_recognition);
    if (m_recall)      fsrs_deck_free(m_recall);
}

// ─────────────────────────────────────────────────────────────────────────────
// initialize
// ─────────────────────────────────────────────────────────────────────────────

void SrsService::initialize(uint32_t dictSize, Configuration *config)
{
    m_config   = config;
    m_dictSize = dictSize;

    fsrs_params params = fsrs_default_params();
    m_recognition = fsrs_deck_create(dictSize, &params);
    m_recall      = fsrs_deck_create(dictSize, &params);

    if (m_config) fsrs_seed(m_config->deviceId);

    uint64_t device_id = m_config ? m_config->deviceId : 0;
    fsrs_sync_init(&m_recogSync,  device_id);
    fsrs_sync_init(&m_recallSync, device_id);

    fsrs_session_start(&m_recogSession,  fsrs_now(), 0);
    fsrs_session_start(&m_recallSession, fsrs_now(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// applyConfig
// ─────────────────────────────────────────────────────────────────────────────

void SrsService::applyConfig(fsrs_deck *deck) const
{
    if (!deck || !m_config) return;

    const auto &ls  = m_config->learningStepsParsed;
    const auto &rls = m_config->relearningStepsParsed;

    update_srs_config(deck,
        nullptr,
        m_config->desiredRetention,
        m_config->maximumInterval,
        m_config->leechThreshold,
        m_config->dayOffset,
        m_config->newCardsPerDay,
        m_config->reviewsPerDay,
        ls.isEmpty()  ? nullptr : ls.constData(),  ls.size(),
        rls.isEmpty() ? nullptr : rls.constData(), rls.size(),
        0, 0,
        m_config->enableFuzz,
        static_cast<fsrs_order_mode>(m_config->orderMode)
    );
}

bool SrsService::compactSync(CardType type, uint64_t now)
{
    if (m_basePath.empty()) return false;
    return fsrs_sync_compact(&syncFor(type), deckFor(type),
                             snapPath(type).c_str(),
                             logPath(type).c_str(), now);
}

// ─────────────────────────────────────────────────────────────────────────────
// _add / _remove / _reset / _suspend / _unsuspend
// ─────────────────────────────────────────────────────────────────────────────

bool SrsService::_add(uint32_t entSeq, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;

    uint64_t now = fsrs_now();
    fsrs_card *card = fsrs_add_card(deck, entSeq, now);
    if (!card) return false;

    if (auto *h = historyFor(type)) h->recordAdd(card, now);

    if (!m_basePath.empty()) {
        FsrsEvent *ev = fsrs_sync_record_add(&syncFor(type), card, now);
        (void)fsrs_sync_log_append(logPath(type).c_str(), ev);
        // Flush inmediato al añadir — garantiza que la carta surviva un crash
        (void)fsrs_save(deck, srsPath(type).c_str());
    }
    return true;
}

bool SrsService::_remove(uint32_t entSeq, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;

    uint64_t now = fsrs_now();
    const fsrs_card *before = fsrs_get_card(deck, entSeq);
    fsrs_card before_copy   = before ? *before : fsrs_card{};
    const bool had_card     = (before != nullptr);

    if (!fsrs_remove_card(deck, entSeq)) return false;

    if (auto *h = historyFor(type))
        if (had_card) h->recordRemove(entSeq, &before_copy, now);

    if (!m_basePath.empty()) {
        FsrsEvent *ev = fsrs_sync_record_remove(&syncFor(type), entSeq, now);
        (void)fsrs_sync_log_append(logPath(type).c_str(), ev);
        (void)fsrs_save(deck, srsPath(type).c_str());
    }
    return true;
}

bool SrsService::_reset(uint32_t entSeq, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;

    uint64_t now = fsrs_now();
    if (!fsrs_remove_card(deck, entSeq)) return false;
    {
        FsrsEvent *ev = fsrs_sync_record_remove(&syncFor(type), entSeq, now);
        (void)fsrs_sync_log_append(logPath(type).c_str(), ev);
    }
    fsrs_card *card = fsrs_add_card(deck, entSeq, now);
    if (!card) return false;
    {
        FsrsEvent *ev = fsrs_sync_record_add(&syncFor(type), card, now);
        (void)fsrs_sync_log_append(logPath(type).c_str(), ev);
    }
    return true;
}

bool SrsService::_suspend(uint32_t entSeq, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;

    uint64_t now = fsrs_now();
    const fsrs_card *sc = fsrs_get_card(deck, entSeq);
    fsrs_card before    = sc ? *sc : fsrs_card{};
    const bool had      = (sc != nullptr);

    fsrs_suspend(deck, entSeq);
    fsrs_rebuild_queues(deck, now);

    if (auto *h = historyFor(type))
        if (had) h->recordSuspend(&before, now);

    return compactSync(type, now);
}

bool SrsService::_unsuspend(uint32_t entSeq, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;

    uint64_t now = fsrs_now();
    fsrs_unsuspend(deck, entSeq, now);
    fsrs_rebuild_queues(deck, now);

    const fsrs_card *after = fsrs_get_card(deck, entSeq);
    if (auto *h = historyFor(type)) h->recordUnsuspend(after, now);

    return compactSync(type, now);
}

// ─────────────────────────────────────────────────────────────────────────────
// API pública — manipulación
// ─────────────────────────────────────────────────────────────────────────────

bool SrsService::add(uint32_t entSeq, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    bool ok = _add(entSeq, type);

    if (ok && type == CardType::Recognition && m_autoAddRecall)
        _add(entSeq, CardType::Recall);

    if (ok) {
        fsrs_rebuild_queues(deckFor(type), fsrs_now());
        if (m_autoAddRecall && type == CardType::Recognition && m_recall)
            fsrs_rebuild_queues(m_recall, fsrs_now());
    }
    return ok;
}

bool SrsService::addBoth(uint32_t entSeq)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    uint64_t now = fsrs_now();
    bool ok1 = _add(entSeq, CardType::Recognition);
    bool ok2 = _add(entSeq, CardType::Recall);
    if (ok1) fsrs_rebuild_queues(m_recognition, now);
    if (ok2) fsrs_rebuild_queues(m_recall,      now);
    return ok1 || ok2;
}

bool SrsService::remove(uint32_t entSeq, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    bool ok = _remove(entSeq, type);
    if (ok) fsrs_rebuild_queues(deckFor(type), fsrs_now());
    return ok;
}

void SrsService::removeBoth(uint32_t entSeq)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    uint64_t now = fsrs_now();
    _remove(entSeq, CardType::Recognition);
    _remove(entSeq, CardType::Recall);
    if (m_recognition) fsrs_rebuild_queues(m_recognition, now);
    if (m_recall)      fsrs_rebuild_queues(m_recall,      now);
}

bool SrsService::contains(uint32_t entSeq, CardType type) const
{
    fsrs_deck *d = deckFor(type);
    return d && fsrs_get_card(d, entSeq) != nullptr;
}

bool SrsService::reset(uint32_t entSeq, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    bool ok = _reset(entSeq, type);
    if (ok) fsrs_rebuild_queues(deckFor(type), fsrs_now());
    return ok;
}

bool SrsService::suspend(uint32_t entSeq, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    return _suspend(entSeq, type);
}

bool SrsService::unsuspend(uint32_t entSeq, CardType type)
{
    std::lock_guard<std::mutex> lock(m_deckMutex);
    return _unsuspend(entSeq, type);
}

bool SrsService::mark(uint32_t entSeq, bool marked, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;
    uint64_t now = fsrs_now();
    fsrs_mark(deck, entSeq, marked);
    fsrs_rebuild_queues(deck, now);
    return compactSync(type, now);
}

bool SrsService::snooze(uint32_t entSeq, uint32_t days, CardType type)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return false;
    uint64_t now = fsrs_now();
    fsrs_snooze(deck, entSeq, days, now);
    fsrs_rebuild_queues(deck, now);
    return compactSync(type, now);
}

// ─────────────────────────────────────────────────────────────────────────────
// sesión
// ─────────────────────────────────────────────────────────────────────────────

void SrsService::startSession(uint32_t limit)
{
    uint64_t now = fsrs_now();
    fsrs_session_start(&m_recogSession,  now, limit);
    fsrs_session_start(&m_recallSession, now, limit);
    m_activeFilter.clear();
    // Restaurar cards_done del día actual si existe
    loadSession();
}

void SrsService::startCustomSession(const QString &customDeckId, uint32_t limit)
{
    uint64_t now = fsrs_now();
    fsrs_session_start(&m_recogSession,  now, limit);
    fsrs_session_start(&m_recallSession, now, limit);
    loadSession();

    m_activeFilter.clear();
    const CustomDeck *cd = m_customDecks.find(customDeckId);
    if (cd) {
        m_activeFilter.reserve(cd->entSeqs.size());
        for (uint32_t seq : cd->entSeqs)
            m_activeFilter.append(seq);
    }
}

NextCard SrsService::nextCard()
{
    if (!m_recognition && !m_recall) return {};

    uint64_t now = fsrs_now();

    if (m_recognition) {
        uint64_t session_day = fsrs_current_day(m_recognition, m_recogSession.session_start);
        uint64_t current_day = fsrs_current_day(m_recognition, now);
        if (session_day != current_day) {
            uint32_t limit = m_recogSession.session_limit;
            fsrs_session_start(&m_recogSession,  now, limit);
            fsrs_session_start(&m_recallSession, now, limit);
            // Nuevo día — contadores a 0, borramos el session file
            saveSession();
        }
    }

    const uint32_t *filter   = m_activeFilter.isEmpty() ? nullptr : m_activeFilter.constData();
    uint32_t        filterSz = static_cast<uint32_t>(m_activeFilter.size());

    NextCard nc = SrsScheduler::next(
        m_recognition, &m_recogSession,
        m_recall,      &m_recallSession,
        now, filter, filterSz);

    if (!nc) return {};
    if (nc.card->state == FSRS_STATE_SUSPENDED) return {};

    uint64_t today = fsrs_current_day(deckFor(nc.type), now);
    if (nc.card->state == FSRS_STATE_REVIEW &&
        (uint64_t)nc.card->due_day > today) return {};

    return nc;
}

// ─────────────────────────────────────────────────────────────────────────────
// answer
// ─────────────────────────────────────────────────────────────────────────────

void SrsService::answer(uint32_t entSeq, CardType type, fsrs_rating rating)
{
    fsrs_deck *deck = deckFor(type);
    if (!deck) return;

    uint64_t now   = fsrs_now();
    fsrs_card *card = getCard(entSeq, type);
    if (!card) return;

    fsrs_session &session = sessionFor(type);

    m_undoStack = UndoEntry{
        .card        = *card,
        .entSeq      = entSeq,
        .type        = type,
        .cards_done  = session.cards_done,
        .new_done    = session.new_done,
        .learn_done  = session.learn_done,
        .review_done = session.review_done
    };

    uint8_t prev_state = card->state;
    fsrs_answer(deck, card, rating, now);

    session.cards_done++;
    if      (prev_state == FSRS_STATE_NEW)                              session.new_done++;
    else if (prev_state == FSRS_STATE_LEARNING ||
             prev_state == FSRS_STATE_RELEARNING)                       session.learn_done++;
    else if (prev_state == FSRS_STATE_REVIEW)                           session.review_done++;

    fsrs_rebuild_queues(deck, now);

    if (auto *h = historyFor(type))
        h->recordAnswer(fsrs_get_card(deck, entSeq), rating, now);

    FsrsEvent *ev = fsrs_sync_record_answer(&syncFor(type), card, rating, now);
    (void)fsrs_sync_log_append(logPath(type).c_str(), ev);

    // Persistir contadores tras cada respuesta
    saveSession();
}

// ─────────────────────────────────────────────────────────────────────────────
// undo
// ─────────────────────────────────────────────────────────────────────────────

bool SrsService::undoLastAnswer()
{
    if (!m_undoStack.has_value()) return false;

    const UndoEntry &undo = *m_undoStack;
    fsrs_deck *deck = deckFor(undo.type);
    if (!deck) return false;

    fsrs_card *card = getCard(undo.entSeq, undo.type);
    if (!card) return false;

    *card = undo.card;
    card->heap_pos_due   = -1;
    card->heap_pos_learn = -1;

    fsrs_session &session = sessionFor(undo.type);
    session.cards_done  = undo.cards_done;
    session.new_done    = undo.new_done;
    session.learn_done  = undo.learn_done;
    session.review_done = undo.review_done;

    uint64_t now = fsrs_now();
    FsrsEvent *ev = fsrs_sync_record_undo(&syncFor(undo.type), undo.entSeq, now);
    if (ev) (void)fsrs_sync_log_append(logPath(undo.type).c_str(), ev);

    fsrs_rebuild_queues(deck, now);

    if (auto *h = historyFor(undo.type))
        h->recordUndo(undo.entSeq, fsrs_get_card(deck, undo.entSeq), now);

    m_undoStack.reset();
    saveSession();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// estadísticas
// ─────────────────────────────────────────────────────────────────────────────

static uint32_t deckDueCount(fsrs_deck *deck, uint64_t now)
{
    if (!deck) return 0;
    uint64_t today = fsrs_current_day(deck, now);
    uint32_t count = 0;
    for (uint32_t i = 0, total = fsrs_deck_count(deck); i < total; ++i) {
        const fsrs_card *c = fsrs_deck_card(deck, i);
        if (!c || c->state == FSRS_STATE_SUSPENDED) continue;
        if ((uint64_t)c->due_day <= today) ++count;
    }
    return count;
}

uint32_t SrsService::totalCount() const
{
    return (m_recognition ? fsrs_deck_count(m_recognition) : 0)
         + (m_recall      ? fsrs_deck_count(m_recall)      : 0);
}

uint32_t SrsService::dueCount() const
{
    uint64_t now = fsrs_now();
    return deckDueCount(m_recognition, now) + deckDueCount(m_recall, now);
}

uint32_t SrsService::learningCount() const
{
    fsrs_stats sr{}, rc{};
    if (m_recognition) fsrs_compute_stats(m_recognition, fsrs_now(), &sr);
    if (m_recall)      fsrs_compute_stats(m_recall,      fsrs_now(), &rc);
    return sr.learning_count + sr.relearning_count
         + rc.learning_count + rc.relearning_count;
}

uint32_t SrsService::newCount() const
{
    fsrs_stats sr{}, rc{};
    if (m_recognition) fsrs_compute_stats(m_recognition, fsrs_now(), &sr);
    if (m_recall)      fsrs_compute_stats(m_recall,      fsrs_now(), &rc);
    return sr.new_count + rc.new_count;
}

uint32_t SrsService::lapsedCount() const
{
    fsrs_stats sr{}, rc{};
    if (m_recognition) fsrs_compute_stats(m_recognition, fsrs_now(), &sr);
    if (m_recall)      fsrs_compute_stats(m_recall,      fsrs_now(), &rc);
    return sr.cards_with_lapses + rc.cards_with_lapses;
}

uint32_t SrsService::reviewTodayCount() const
{
    return m_recogSession.cards_done + m_recallSession.cards_done;
}

uint64_t SrsService::waitSeconds() const
{
    uint64_t now = fsrs_now();
    uint64_t wr  = m_recognition ? fsrs_wait_seconds(m_recognition, now) : UINT64_MAX;
    uint64_t wrc = m_recall      ? fsrs_wait_seconds(m_recall,      now) : UINT64_MAX;
    uint64_t w   = (wr < wrc) ? wr : wrc;
    return (w == UINT64_MAX) ? 0 : w;
}

// ─────────────────────────────────────────────────────────────────────────────
// preview / texto
// ─────────────────────────────────────────────────────────────────────────────

std::string SrsService::predictInterval(uint32_t entSeq, CardType type, fsrs_rating rating)
{
    fsrs_card *card = getCard(entSeq, type);
    fsrs_deck *deck = deckFor(type);
    if (!card || !deck) return {};

    uint64_t now = fsrs_now();
    uint64_t out[4] = {};
    fsrs_preview(deck, card, now, out);

    int idx = static_cast<int>(rating) - 1;
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;

    uint64_t delta = (out[idx] > now) ? (out[idx] - now) : 0;
    char buf[64] = {};
    fsrs_format_interval(delta, buf, sizeof(buf));
    return std::string(buf);
}

std::string SrsService::dueDateText(uint32_t entSeq, CardType type) const
{
    fsrs_card *card = getCard(entSeq, type);
    if (!card) return {};

    uint64_t now = fsrs_now();
    if (card->state == FSRS_STATE_NEW)       return "New";
    if (card->state == FSRS_STATE_SUSPENDED) return "Suspended";
    if (card->due_ts <= now)                 return "Now";

    char buf[64] = {};
    fsrs_format_interval(card->due_ts - now, buf, sizeof(buf));
    return std::string(buf);
}

std::string SrsService::stateText(uint32_t entSeq, CardType type) const
{
    fsrs_card *card = getCard(entSeq, type);
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

// ─────────────────────────────────────────────────────────────────────────────
// updateConfig
// ─────────────────────────────────────────────────────────────────────────────

void SrsService::updateConfig(const Configuration *config)
{
    if (!config) return;
    m_config = config;
    setAutoAddRecall(config->autoAddRecall);
    applyConfig(m_recognition);
    applyConfig(m_recall);
}

// ─────────────────────────────────────────────────────────────────────────────
// persistencia
// ─────────────────────────────────────────────────────────────────────────────

static fsrs_deck *loadOrCreate(const std::string &srsFile,
                                const std::string &snapFile,
                                const std::string &logFile,
                                FsrsSync          &sync,
                                uint32_t           dictSize,
                                const fsrs_params &params)
{
    fsrs_deck *deck = fsrs_load(srsFile.c_str(), &params);
    if (!deck)
        deck = fsrs_deck_create(dictSize, &params);
    if (!deck) return nullptr;

    (void)fsrs_sync_snapshot_load(deck, snapFile.c_str(), fsrs_now());
    (void)fsrs_sync_log_load(&sync, logFile.c_str());
    fsrs_sync_rebuild(&sync, deck, fsrs_now());
    fsrs_rebuild_queues(deck, fsrs_now());
    return deck;
}

bool SrsService::load(const QString &basePath)
{
    // basePath es el prefijo sin extensión, p.ej. ".../profile"
    m_basePath = basePath.toStdString();

    fsrs_params params = fsrs_default_params();

    if (m_recognition) { fsrs_deck_free(m_recognition); m_recognition = nullptr; }
    if (m_recall)      { fsrs_deck_free(m_recall);      m_recall      = nullptr; }
    fsrs_sync_free(&m_recogSync);
    fsrs_sync_free(&m_recallSync);

    uint64_t device_id = m_config ? m_config->deviceId : 0;
    fsrs_sync_init(&m_recogSync,  device_id);
    fsrs_sync_init(&m_recallSync, device_id);

    m_recognition = loadOrCreate(
        srsPath(CardType::Recognition),
        snapPath(CardType::Recognition),
        logPath(CardType::Recognition),
        m_recogSync, m_dictSize, params);

    m_recall = loadOrCreate(
        srsPath(CardType::Recall),
        snapPath(CardType::Recall),
        logPath(CardType::Recall),
        m_recallSync, m_dictSize, params);

    if (!m_recognition || !m_recall) return false;

    applyConfig(m_recognition);
    applyConfig(m_recall);

    m_recogHistory  = std::make_unique<SrsHistoryLog>(m_basePath + ".recog");
    m_recallHistory = std::make_unique<SrsHistoryLog>(m_basePath + ".recall");

    m_customDecks.load(QString::fromStdString(customDecksPath()));

    uint32_t prev_limit = m_recogSession.session_limit;
    fsrs_session_start(&m_recogSession,  fsrs_now(), prev_limit);
    fsrs_session_start(&m_recallSession, fsrs_now(), prev_limit);

    // Restaurar contadores del día
    loadSession();

    return true;
}

bool SrsService::save()
{
    if (m_basePath.empty()) return false;

    bool ok = true;
    if (m_recognition)
        ok &= fsrs_save(m_recognition, srsPath(CardType::Recognition).c_str())
           && fsrs_sync_compact(&m_recogSync, m_recognition,
                                snapPath(CardType::Recognition).c_str(),
                                logPath(CardType::Recognition).c_str(),
                                fsrs_now());

    if (m_recall)
        ok &= fsrs_save(m_recall, srsPath(CardType::Recall).c_str())
           && fsrs_sync_compact(&m_recallSync, m_recall,
                                snapPath(CardType::Recall).c_str(),
                                logPath(CardType::Recall).c_str(),
                                fsrs_now());

    saveSession();
    return ok;
}

void SrsService::saveAsync()
{
    if (m_basePath.empty()) return;

    std::string base = m_basePath;

    QThreadPool::globalInstance()->start([this, base]() {
        std::lock_guard<std::mutex> lock(m_deckMutex);
        uint64_t now = fsrs_now();

        if (m_recognition) {
            (void)fsrs_save(m_recognition, (base + ".recog.srs").c_str());
            (void)fsrs_sync_compact(&m_recogSync, m_recognition,
                                    (base + ".recog.sync.snap").c_str(),
                                    (base + ".recog.sync.log").c_str(), now);
        }
        if (m_recall) {
            (void)fsrs_save(m_recall, (base + ".recall.srs").c_str());
            (void)fsrs_sync_compact(&m_recallSync, m_recall,
                                    (base + ".recall.sync.snap").c_str(),
                                    (base + ".recall.sync.log").c_str(), now);
        }
        saveSession();
    });
}