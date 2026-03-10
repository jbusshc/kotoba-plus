#include "SrsService.h"
#include "../app/Configuration.h"
#include <cstring>
#include <string>
#include <sstream>
#include <cstdlib>

SrsService::SrsService(uint32_t dictSize, Configuration *config)
    : m_dictSize(dictSize), m_config(config),
      m_lastPoppedIndex(-1), m_lastPoppedEntryId(0)
{
    srs_init(&m_profile, dictSize);
    srs_heapify(&m_profile);

    // generate a decent device id if config doesn't provide one
    uint64_t device_id =  config->deviceId;
    srs_sync_init(&m_sync, device_id, dictSize);
}

SrsService::~SrsService()
{
    // persist/cleanup sync structures if needed
    srs_sync_free(&m_sync);
    srs_free(&m_profile);
}

static inline std::string snapshot_path_for(const char* base)
{
    return std::string(base) + ".sync.snap";
}
static inline std::string log_path_for(const char* base)
{
    return std::string(base) + ".sync.log";
}

bool SrsService::load(const char *path)
{
    bool ok = srs_load(&m_profile, path, m_dictSize);
    if (!ok) return false;

    // try to load sync snapshot + log (non-fatal)
    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    (void)srs_snapshot_load(&m_sync, snap.c_str());
    (void)srs_log_load(&m_sync, log.c_str());

    // rebuild sync derived state
    srs_sync_rebuild(&m_sync);

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
    return srs_add(&m_profile, entryId, srs_now());
}

bool SrsService::remove(uint32_t entryId)
{
    return srs_remove(&m_profile, entryId);
}

bool SrsService::contains(uint32_t entryId) const
{
    return srs_contains(&m_profile, entryId);
}

std::optional<srs_review> SrsService::nextDue()
{
    srs_review out;
    if (srs_pop_due_review(&m_profile, &out)) {
        // store index so answer() can requeue efficiently
        m_lastPoppedIndex = (int32_t)out.index;
        m_lastPoppedEntryId = out.item ? out.item->entry_id : 0;
        return out;
    }
    m_lastPoppedIndex = -1;
    return std::nullopt;
}

void SrsService::answer(uint32_t entryId, srs_quality q)
{
    uint64_t now = srs_now();

    // If user is answering the last popped card, requeue using its index
    if (m_lastPoppedIndex >= 0 &&
        m_profile.items[m_lastPoppedIndex].entry_id == entryId)
    {
        srs_answer(&m_profile.items[m_lastPoppedIndex], q, now);
        srs_requeue(&m_profile, (uint32_t)m_lastPoppedIndex);
        m_lastPoppedIndex = -1;
    }
    else
    {
        // fallback: find by entry id and apply answer, then rebuild heap
        for (uint32_t i = 0; i < m_profile.count; ++i)
        {
            if (m_profile.items[i].entry_id == entryId)
            {
                srs_answer(&m_profile.items[i], q, now);
                srs_heapify(&m_profile);
                break;
            }
        }
    }

    // register local review in sync state (timestamp = now)
    srs_sync_add_local_review(&m_sync, entryId, (uint8_t)q, now);

    // persist log
    std::string log = log_path_for(m_config->srsPath.toStdString().c_str());
    (void)srs_log_append(log.c_str(), &m_sync.events[m_sync.event_count - 1]);
}

uint32_t SrsService::dueCount(uint64_t now) const
{
    srs_stats st;
    srs_compute_stats(&m_profile, now, &st);
    return st.due_now;
}

uint32_t SrsService::learningCount() const
{
    // count by state: LEARNING or RELEARNING are "learning"
    uint32_t c = 0;
    for (uint32_t i = 0; i < m_profile.count; ++i) {
        uint8_t s = m_profile.items[i].state;
        if (s == SRS_STATE_LEARNING || s == SRS_STATE_RELEARNING)
            ++c;
    }
    return c;
}

uint32_t SrsService::newCount() const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < m_profile.count; ++i)
        if (m_profile.items[i].state == SRS_STATE_NEW) ++c;
    return c;
}

uint32_t SrsService::lapsedCount() const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < m_profile.count; ++i)
        if (m_profile.items[i].lapses > 0) ++c;
    return c;
}