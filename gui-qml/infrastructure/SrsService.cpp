#include "SrsService.h"
#include "../app/Configuration.h"

#include <algorithm>
#include <random>
#include <string>

static inline std::string snapshot_path_for(const char* base)
{
    return std::string(base) + ".sync.snap";
}

static inline std::string log_path_for(const char* base)
{
    return std::string(base) + ".sync.log";
}

static uint64_t today_stamp()
{
    return srs_now() / 86400;
}

SrsService::SrsService(uint32_t dictSize, Configuration *config)
    : m_dictSize(dictSize), m_config(config)
{
    srs_init(&m_profile, dictSize);
    srs_heapify(&m_profile);

    uint64_t device_id = config->deviceId;
    srs_sync_init(&m_sync, device_id, dictSize);

    m_idIndex.resize(dictSize, -1);

    m_dayStamp = today_stamp();
}

SrsService::~SrsService()
{
    srs_sync_free(&m_sync);
    srs_free(&m_profile);
}

int32_t SrsService::indexOf(uint32_t entryId) const
{
    if (entryId >= m_idIndex.size())
        return -1;

    return m_idIndex[entryId];
}

void SrsService::resetDailyLimits()
{
    uint64_t today = today_stamp();

    if (today != m_dayStamp)
    {
        m_dayStamp = today;
        m_newToday = 0;
        rebuildNewQueue();
    }
}

void SrsService::rebuildNewQueue()
{
    m_newQueue.clear();

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        if (m_profile.items[i].state == SRS_STATE_NEW)
            m_newQueue.push_back(m_profile.items[i].entry_id);
    }

    static std::mt19937 rng(std::random_device{}());
    std::shuffle(m_newQueue.begin(), m_newQueue.end(), rng);
}

void SrsService::rebuildIndex()
{
    std::fill(m_idIndex.begin(), m_idIndex.end(), -1);

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        uint32_t id = m_profile.items[i].entry_id;

        if (id < m_idIndex.size())
            m_idIndex[id] = i;
    }
}

void SrsService::applySyncEventsToProfile()
{
    rebuildIndex();

    for (size_t i = 0; i < m_sync.event_count; ++i)
    {
        const SrsEvent *ev = &m_sync.events[i];

        uint32_t id = ev->card_id;

        if (id >= m_dictSize)
            continue;

        switch (ev->type)
        {

        case SRS_EVENT_ADD:
        {
            if (!srs_contains(&m_profile, id))
            {
                if (srs_add(&m_profile, id, ev->timestamp))
                    m_idIndex[id] = m_profile.count - 1;
            }
            break;
        }

        case SRS_EVENT_REMOVE:
        {
            int32_t idx = indexOf(id);
            if (idx >= 0)
            {
                srs_remove(&m_profile, id);
                rebuildIndex();
            }
            break;
        }

        case SRS_EVENT_REVIEW:
        {
            int32_t idx = indexOf(id);
            if (idx < 0)
                break;

            srs_item *it = &m_profile.items[idx];
            srs_answer(it, (srs_quality)ev->grade, ev->timestamp);

            break;
        }

        default:
            break;
        }
    }

    srs_heapify(&m_profile);
    rebuildIndex();
    rebuildNewQueue();

    m_dueQueue.clear();

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        if (m_profile.items[i].state == SRS_STATE_LEARNING ||
            m_profile.items[i].state == SRS_STATE_RELEARNING)
        {
            m_dueQueue.push_back(m_profile.items[i].entry_id);
        }
    }
}

bool SrsService::load(const char *path)
{
    bool ok = srs_load(&m_profile, path, m_dictSize);
    if (!ok) return false;

    m_profilePath = path;

    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    (void)srs_snapshot_load(&m_sync, snap.c_str());
    (void)srs_log_load(&m_sync, log.c_str());

    srs_sync_rebuild(&m_sync);

    applySyncEventsToProfile();

    (void)srs_save(&m_profile, path);

    return true;
}

bool SrsService::save(const char *path)
{
    if (!srs_save(&m_profile, path))
        return false;

    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    return srs_compact(&m_sync, snap.c_str(), log.c_str());
}

bool SrsService::add(uint32_t entryId)
{
    uint64_t now = srs_now();

    if (!srs_add(&m_profile, entryId, now))
        return false;

    m_idIndex[entryId] = m_profile.count - 1;

    srs_sync_add_local_add(&m_sync, entryId, now);

    std::string base = m_profilePath.empty() ? m_config->srsPath.toStdString() : m_profilePath;
    std::string log = log_path_for(base.c_str());

    (void)srs_log_append(log.c_str(),
                         &m_sync.events[m_sync.event_count - 1]);

    srs_heapify(&m_profile);
    rebuildIndex();

    m_newQueue.push_back(entryId);

    return true;
}

bool SrsService::remove(uint32_t entryId)
{
    int32_t idx = indexOf(entryId);
    if (idx < 0)
        return false;

    bool ok = srs_remove(&m_profile, entryId);

    if (ok)
    {
        uint64_t now = srs_now();

        srs_sync_add_local_remove(&m_sync, entryId, now);

        std::string base = m_profilePath.empty() ? m_config->srsPath.toStdString() : m_profilePath;
        std::string log = log_path_for(base.c_str());

        (void)srs_log_append(log.c_str(),
                             &m_sync.events[m_sync.event_count - 1]);

        rebuildIndex();
        rebuildNewQueue();
    }

    return ok;
}

bool SrsService::contains(uint32_t entryId) const
{
    return indexOf(entryId) >= 0;
}

std::optional<srs_review> SrsService::nextDue()
{
    resetDailyLimits();

    uint64_t now = srs_now();

    // 1️⃣ revisar si hay cartas de learning/relearning pendientes
    while (!m_dueQueue.empty())
    {
        uint32_t id = m_dueQueue.back();
        m_dueQueue.pop_back();

        int32_t idx = indexOf(id);
        if (idx >= 0)
        {
            srs_item *item = &m_profile.items[idx];
            // solo se mantiene si sigue siendo learning/relearning
            if (item->state == SRS_STATE_LEARNING ||
                item->state == SRS_STATE_RELEARNING)
            {
                srs_review out;
                out.item = item;
                out.index = idx;
                return out;
            }
        }
    }

    // 2️⃣ revisar cartas con repaso debido (review)
    srs_item *it = srs_peek_due(&m_profile, now);
    if (it)
    {
        srs_review out;
        out.item = it;
        out.index = (uint32_t)(it - m_profile.items);
        return out;
    }

    // 3️⃣ revisar cartas nuevas
    if (m_newToday >= m_newLimit || m_newQueue.empty())
        return std::nullopt;

    uint32_t id = m_newQueue.back();
    m_newQueue.pop_back();

    int32_t idx = indexOf(id);
    if (idx < 0)
        return std::nullopt;

    srs_review out;
    out.item = &m_profile.items[idx];
    out.index = idx;

    return out;
}

void SrsService::answer(uint32_t entryId, srs_quality q)
{
    uint64_t now = srs_now();

    int32_t idx = indexOf(entryId);
    if (idx < 0) return;

    srs_item *item = &m_profile.items[idx];

    if (item->state == SRS_STATE_NEW)
        m_newToday++;

    srs_answer(item, q, now);

    // 🔹 si la carta sigue siendo learning/relearning, reingresarla a la sesión
    if (item->state == SRS_STATE_LEARNING ||
        item->state == SRS_STATE_RELEARNING)
    {
        m_dueQueue.push_back(entryId);
    }

    srs_heapify(&m_profile);
    rebuildIndex();

    srs_sync_add_local_review(&m_sync, entryId, (uint8_t)q, now);

    std::string base = m_profilePath.empty() ? m_config->srsPath.toStdString() : m_profilePath;
    std::string log = log_path_for(base.c_str());

    (void)srs_log_append(log.c_str(),
                         &m_sync.events[m_sync.event_count - 1]);
}

uint32_t SrsService::dueCount(uint64_t now) const
{
    srs_stats st;
    srs_compute_stats(&m_profile, now, &st);
    return st.due_now;
}

uint32_t SrsService::learningCount() const
{
    uint32_t c = 0;

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        uint8_t s = m_profile.items[i].state;

        if (s == SRS_STATE_LEARNING ||
            s == SRS_STATE_RELEARNING)
            ++c;
    }

    return c;
}

uint32_t SrsService::newCount() const
{
    return m_newQueue.size();
}

uint32_t SrsService::lapsedCount() const
{
    uint32_t c = 0;

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        if (m_profile.items[i].lapses > 0)
            ++c;
    }

    return c;
}