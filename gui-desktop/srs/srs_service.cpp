#include "srs_service.h"

SrsService::SrsService(uint32_t dictSize)
    : dictSize(dictSize)
{
    srs_init(&profile, dictSize);
}

SrsService::~SrsService()
{
    srs_free(&profile);
}

bool SrsService::load(const QString& snapshotPath)
{
    return srs_load(&profile,
                    snapshotPath.toUtf8().constData(),
                    dictSize);
}

bool SrsService::save(const QString& snapshotPath)
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
    srs_review review;

    if (!srs_pop_due_review(&profile, &review))
        return std::nullopt;

    ReviewItem item;
    item.entryId = review.item->entry_id;
    item.interval = review.item->interval;
    item.ease = review.item->ease;
    item.state = review.item->state;

    srs_requeue(&profile, review.index);

    return item;
}

void SrsService::answer(uint32_t entryId, srs_quality quality)
{
    for (uint32_t i = 0; i < profile.count; ++i)
    {
        if (profile.items[i].entry_id == entryId)
        {
            srs_answer(&profile.items[i], quality, srs_now());
            break;
        }
    }
}

uint32_t SrsService::dueCount(uint64_t now) const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < profile.count; ++i) {
        if (profile.items[i].due <= now)
            ++c;
    }
    return c;
}

uint32_t SrsService::learningCount() const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < profile.count; ++i) {
        // si la librería soporta SRS_LEARNING
        if (profile.items[i].state == SRS_LEARNING)
            ++c;
    }
    return c;
}

uint32_t SrsService::newCount() const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < dictSize; ++i) {
        // el bitmap marca los elementos incluidos en SRS
        bool in_srs = (profile.bitmap[i >> 3] >> (i & 7)) & 1;
        if (!in_srs) ++c;
    }
    return c;
}

uint32_t SrsService::lapsedCount() const
{
    uint32_t c = 0;
    for (uint32_t i = 0; i < profile.count; ++i) {
        if (profile.items[i].lapses > 0)
            ++c;
    }
    return c;
}