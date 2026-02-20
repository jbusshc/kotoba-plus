#include "srs_service.h"

SrsService::SrsService(uint32_t dictSize)
    : dictSize(dictSize)
{
    srs_init(&profile, dictSize);
    initSessionSeenBitset(dictSize);
}

SrsService::~SrsService()
{
    srs_free(&profile);
    cleanupSessionSeenBitset();
}

bool SrsService::load(const QString &snapshotPath)
{
    return srs_load(&profile,
                    snapshotPath.toUtf8().constData(),
                    dictSize);
}

bool SrsService::save(const QString &snapshotPath)
{
    return srs_save(&profile,
                    snapshotPath.toUtf8().constData());
}

bool SrsService::add(uint32_t entryId)
{
    return srs_add(&profile, entryId, srs_now());
}

bool SrsService::remove(uint32_t entryId)
{
    return srs_remove(&profile, entryId);
}

bool SrsService::contains(uint32_t entryId) const
{
    return srs_contains(&profile, entryId);
}

std::optional<SrsService::ReviewItem> SrsService::nextDue()
{
    uint64_t now = srs_now();

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        auto &item = profile.items[i];

        // Mostrar si:
        // - Está en learning
        // - O está due
        if (item.state == SRS_STATE_LEARNING ||
            item.due <= now)
        {
            ReviewItem r;
            r.entryId = item.entry_id;
            r.interval = item.interval;
            r.ease = item.ease;
            r.state = item.state;
            return r;
        }
    }

    return std::nullopt;
}


void SrsService::answer(uint32_t entryId, srs_quality quality)
{
    uint64_t now = srs_now();

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        if (profile.items[i].entry_id == entryId)
        {
            srs_answer(&profile.items[i], quality, now);

            // 🔥 CRÍTICO: reconstruir heap
            srs_heapify(&profile);
            return;
        }
    }
}

uint32_t SrsService::dueCount(uint64_t now) const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < profile.count; ++i)
    {
        if (profile.items[i].due <= now)
            ++c;
    }
    return c;
}

uint32_t SrsService::learningCount() const
{
    uint32_t c = 0;

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        // En v2 consideramos learning si intervalo < 1 día
        if (profile.items[i].interval < 86400)
            ++c;
    }

    return c;
}

uint32_t SrsService::newCount() const
{
    uint32_t c = 0;

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        // carta en estado inicial
        if (profile.items[i].state == SRS_STATE_NEW)
            ++c;
    }

    return c;
}

uint32_t SrsService::lapsedCount() const
{
    uint32_t c = 0;

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        if (profile.items[i].lapses > 0)
            ++c;
    }

    return c;
}

bool SrsService::exportEvents(const QString &path)
{
    // placeholder para futuro srs_sync_log_save
    Q_UNUSED(path);
    return true;
}

bool SrsService::importEvents(const QString &path)
{
    // placeholder para futuro srs_sync_merge
    Q_UNUSED(path);
    return true;
}

void SrsService::initSessionSeenBitset(uint32_t dictSize)
{
    sessionSeenBitsetSize = (dictSize + 7) / 8;
    sessionSeenBitset = new char[sessionSeenBitsetSize];
    memset(sessionSeenBitset, 0, sessionSeenBitsetSize);
}

void SrsService::cleanupSessionSeenBitset()
{
    delete[] sessionSeenBitset;
    sessionSeenBitset = nullptr;
    sessionSeenBitsetSize = 0;
}

bool SrsService::isSessionSeen(uint32_t entryId) const
{
    if (entryId >= sessionSeenBitsetSize * 8)
        return false;

    uint32_t byteIdx = entryId / 8;
    uint32_t bitIdx = entryId % 8;
    return (sessionSeenBitset[byteIdx] & (1 << bitIdx)) != 0;
}

void SrsService::markSessionSeen(uint32_t entryId)
{
    if (entryId >= sessionSeenBitsetSize * 8)
        return;

    uint32_t byteIdx = entryId / 8;
    uint32_t bitIdx = entryId % 8;
    sessionSeenBitset[byteIdx] |= (1 << bitIdx);
}

void SrsService::resetSessionSeenBitset()
{
    memset(sessionSeenBitset, 0, sessionSeenBitsetSize);
}

uint32_t SrsService::countDueAndLearningToday() const
{
    uint64_t now = srs_now();
    uint32_t c = 0;

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        const auto &item = profile.items[i];

        if (item.state == SRS_STATE_LEARNING ||
            item.due <= now)
        {
            ++c;
        }
    }

    return c;
}

bool SrsService::isDueToday(uint32_t entryId) const
{
    uint64_t now = srs_now();

    for (uint32_t i = 0; i < profile.count; ++i)
    {
        if (profile.items[i].entry_id == entryId)
        {
            const auto &item = profile.items[i];

            return (item.state == SRS_STATE_LEARNING ||
                    item.due <= now);
        }
    }

    return false;
}
