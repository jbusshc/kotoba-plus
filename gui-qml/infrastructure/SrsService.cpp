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
    srs_init(&m_profile, dictSize);
    srs_heapify(&m_profile);

    uint64_t device_id = config->deviceId;
    srs_sync_init(&m_sync, device_id, dictSize);

    m_idIndex.resize(dictSize, -1);
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

bool SrsService::load(const char *path)
{
    bool ok = srs_load(&m_profile, path, m_dictSize);
    if (!ok) return false;

    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    (void)srs_snapshot_load(&m_sync, snap.c_str());
    (void)srs_log_load(&m_sync, log.c_str());

    srs_sync_rebuild(&m_sync);

    // rebuild index
    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        uint32_t id = m_profile.items[i].entry_id;
        if (id < m_idIndex.size())
            m_idIndex[id] = i;
    }

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
    if (!srs_add(&m_profile, entryId, srs_now()))
        return false;

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        if (m_profile.items[i].entry_id == entryId)
        {
            m_idIndex[entryId] = i;
            break;
        }
    }

    return true;
}

bool SrsService::remove(uint32_t entryId)
{
    int32_t idx = indexOf(entryId);
    if (idx < 0)
        return false;

    bool ok = srs_remove(&m_profile, entryId);

    if (ok)
        m_idIndex[entryId] = -1;

    return ok;
}

bool SrsService::contains(uint32_t entryId) const
{
    return indexOf(entryId) >= 0;
}

std::optional<srs_review> SrsService::nextDue()
{
    uint64_t now = srs_now();

    srs_item *it = srs_peek_due(&m_profile, now);

    if (!it)
        return std::nullopt;

    srs_review out;
    out.item = it;
    out.index = (uint32_t)(it - m_profile.items); // calcular índice

    return out;
}

void SrsService::answer(uint32_t entryId, srs_quality q)
{
    uint64_t now = srs_now();

    int32_t idx = indexOf(entryId);
    if (idx < 0)
        return;

    srs_item *item = &m_profile.items[idx];

    srs_answer(item, q, now);

    // heap reorder
    srs_heapify(&m_profile);

    srs_sync_add_local_review(&m_sync, entryId, (uint8_t)q, now);

    std::string log = log_path_for(m_config->srsPath.toStdString().c_str());
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
    uint32_t c = 0;

    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        if (m_profile.items[i].state == SRS_STATE_NEW)
            ++c;
    }

    return c;
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