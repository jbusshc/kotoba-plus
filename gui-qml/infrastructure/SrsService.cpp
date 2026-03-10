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

void SrsService::rebuildIndex()
{
    // reset
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
    // el profile viene cargado desde profile.bin
    // los eventos ya vienen ordenados por srs_sync_rebuild()

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
                {
                    m_idIndex[id] = m_profile.count - 1;
                }
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

    // solo ahora reconstruimos el heap completo
    srs_heapify(&m_profile);

    // reconstruir índice final
    rebuildIndex();
}

bool SrsService::load(const char *path)
{
    bool ok = srs_load(&m_profile, path, m_dictSize);
    if (!ok) return false;

    // store base path for later saves/log appends
    m_profilePath = path;

    std::string snap = snapshot_path_for(path);
    std::string log = log_path_for(path);

    (void)srs_snapshot_load(&m_sync, snap.c_str());
    (void)srs_log_load(&m_sync, log.c_str());

    // rebuild sync LWW state and sort events
    srs_sync_rebuild(&m_sync);

    // apply events (adds/removes/reviews) to profile so profile reflects snapshot+log
    applySyncEventsToProfile();

    // if we want to persist the applied changes into the profile file (optional),
    // perform a save so next load doesn't need to replay same events
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

    // append an ADD event to sync and persist to log (append-only)
    srs_sync_add_local_add(&m_sync, entryId, now);

    // Base path seguro (evita c_str() de temporales)
    std::string base = m_profilePath.empty() ? m_config->srsPath.toStdString() : m_profilePath;
    std::string log = log_path_for(base.c_str());

    (void)srs_log_append(log.c_str(),
                         &m_sync.events[m_sync.event_count - 1]);

    // rebuild index because heap may have changed
    srs_heapify(&m_profile);
    rebuildIndex();

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
        // append REMOVE event
        uint64_t now = srs_now();
        srs_sync_add_local_remove(&m_sync, entryId, now);

        std::string base = m_profilePath.empty() ? m_config->srsPath.toStdString() : m_profilePath;
        std::string log = log_path_for(base.c_str());
        (void)srs_log_append(log.c_str(),
                             &m_sync.events[m_sync.event_count - 1]);

        // rebuild index after heapify done inside srs_remove
        rebuildIndex();
    }

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

    // heap reorder -> srs_heapify invalida índices, así que reconstruimos
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