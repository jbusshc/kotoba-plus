#ifndef SRS_SERVICE_H
#define SRS_SERVICE_H

#include <QString>
#include <optional>
#include <memory>

extern "C"
{
#include "srs.h"
}

class SrsService
{
public:
    SrsService(uint32_t dictSize);
    ~SrsService();

    bool load(const QString &snapshotPath);
    bool save(const QString &snapshotPath);

    bool add(uint32_t entryId);
    bool remove(uint32_t entryId);
    bool contains(uint32_t entryId) const;
    // añade a la clase SrsService (deja el resto igual)
    uint32_t dueCount(uint64_t now) const;
    uint32_t learningCount() const;
    uint32_t newCount() const;
    uint32_t lapsedCount() const;
    uint32_t countDueAndLearningToday() const;
    bool isDueToday(uint32_t entryId) const;

    bool exportEvents(const QString &path);
    bool importEvents(const QString &path);

    struct ReviewItem
    {
        uint32_t entryId;
        uint16_t interval;
        float ease;
        uint8_t state;
    };

    std::optional<ReviewItem> nextDue();
    void answer(uint32_t entryId, srs_quality quality);
    bool isSessionSeen(uint32_t entryId) const;
    void markSessionSeen(uint32_t entryId);
    void resetSessionSeenBitset();


private:

    void initSessionSeenBitset(uint32_t dictSize);
    void cleanupSessionSeenBitset();

    uint32_t sessionSeenBitsetSize = 0;
    char *sessionSeenBitset = nullptr;

    srs_profile profile;
    uint32_t dictSize;
};

#endif
