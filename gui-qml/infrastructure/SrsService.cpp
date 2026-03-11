#include "SrsService.h"
#include "../app/Configuration.h"

#include <algorithm>
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

    // default to FSRS for now
    setButtonMode(SRS_BUTTONS_6);
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
            if (indexOf(id) >= 0)
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

            srs_answer(&m_profile, it, (srs_quality)ev->grade, ev->timestamp);
            srs_requeue(&m_profile, idx);

            break;
        }

        default:
            break;
        }
    }

    srs_heapify(&m_profile);
    rebuildIndex();
}

bool SrsService::load(const char *path)
{
    if (!srs_load(&m_profile, path, m_dictSize))
        return false;

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

    rebuildIndex();

    srs_sync_add_local_add(&m_sync, entryId, now);

    std::string base = m_profilePath.empty()
        ? m_config->srsPath.toStdString()
        : m_profilePath;

    std::string log = log_path_for(base.c_str());

    (void)srs_log_append(
        log.c_str(),
        &m_sync.events[m_sync.event_count - 1]);

    srs_heapify(&m_profile);

    return true;
}

bool SrsService::remove(uint32_t entryId)
{
    int32_t idx = indexOf(entryId);

    if (idx < 0)
        return false;

    if (!srs_remove(&m_profile, entryId))
        return false;

    uint64_t now = srs_now();

    srs_sync_add_local_remove(&m_sync, entryId, now);

    std::string base = m_profilePath.empty()
        ? m_config->srsPath.toStdString()
        : m_profilePath;

    std::string log = log_path_for(base.c_str());

    (void)srs_log_append(
        log.c_str(),
        &m_sync.events[m_sync.event_count - 1]);

    rebuildIndex();

    return true;
}

bool SrsService::contains(uint32_t entryId) const
{
    return indexOf(entryId) >= 0;
}

std::optional<srs_review> SrsService::nextDue()
{
    uint64_t now = srs_now();

    srs_review out;

    if (!srs_pop_due_review(&m_profile, &out))
        return std::nullopt;

    return out;
}

void SrsService::answer(uint32_t entryId, srs_quality q)
{
    uint64_t now = srs_now();

    int32_t idx = indexOf(entryId);

    if (idx < 0)
        return;

    srs_item *item = &m_profile.items[idx];

    srs_answer(&m_profile, item, q, now);

    srs_requeue(&m_profile, idx);

    srs_sync_add_local_review(&m_sync, entryId, (uint8_t)q, now);

    std::string base = m_profilePath.empty()
        ? m_config->srsPath.toStdString()
        : m_profilePath;

    std::string log = log_path_for(base.c_str());

    (void)srs_log_append(
        log.c_str(),
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

void SrsService::setButtonMode(srs_button_mode mode)
{
    srs_set_button_mode(&m_profile, mode);
}

std::string SrsService::predictInterval(uint32_t entryId, srs_quality q)
{
    int32_t idx = indexOf(entryId);
    if (idx < 0) return "";

    srs_item *it = &m_profile.items[idx];

    uint64_t now = srs_now();

    uint64_t due = srs_predict_due(&m_profile, it, q, now);

    char buf[32];
    srs_format_interval(due - now, buf, sizeof(buf));

    return std::string(buf);
}

int SrsService::sixButtons() const
{
    return (int)m_profile.button_mode == SRS_BUTTONS_6;
}   