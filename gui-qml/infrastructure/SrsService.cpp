#include "SrsService.h"
#include <cstring>

SrsService::SrsService(uint32_t dictSize, Configuration *config)
    : m_dictSize(dictSize), m_config(config)
{
    srs_init(&m_profile, dictSize);
    srs_heapify(&m_profile);
}

SrsService::~SrsService()
{
    srs_free(&m_profile);
}

bool SrsService::load(const char *path)
{
    return srs_load(&m_profile, path, m_dictSize);
}

bool SrsService::save(const char *path)
{
    return srs_save(&m_profile, path);
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
    if (srs_pop_due_review(&m_profile, &out))
        return out;
    return std::nullopt;
}

void SrsService::answer(uint32_t entryId, srs_quality q)
{
    for (uint32_t i = 0; i < m_profile.count; ++i)
    {
        if (m_profile.items[i].entry_id == entryId)
        {
            srs_answer(&m_profile.items[i], q, srs_now());
            srs_heapify(&m_profile);
            return;
        }
    }
}

uint32_t SrsService::dueCount(uint64_t now) const
{
    srs_stats st;
    srs_compute_stats(&m_profile, now, &st);
    return st.due_now;
}

uint32_t SrsService::learningCount() const
{
    // fallback: count items with interval < 1 day (like previous design)
    uint32_t c = 0;
    for (uint32_t i = 0; i < m_profile.count; ++i) {
        if (m_profile.items[i].interval < 86400)
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