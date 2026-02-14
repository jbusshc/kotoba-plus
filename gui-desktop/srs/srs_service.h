#ifndef SRS_SERVICE_H
#define SRS_SERVICE_H

#include <QString>
#include <optional>
#include <memory>

extern "C" {
#include "srs.h"
}

class SrsService
{
public:
    SrsService(uint32_t dictSize);
    ~SrsService();

    bool load(const QString& snapshotPath);
    bool save(const QString& snapshotPath);

    bool add(uint32_t entryId);
    bool remove(uint32_t entryId);
    bool contains(uint32_t entryId) const;

    struct ReviewItem {
        uint32_t entryId;
        uint16_t interval;
        float ease;
        uint8_t state;
    };

    std::optional<ReviewItem> nextDue();
    void answer(uint32_t entryId, srs_quality quality);

private:
    srs_profile profile;
    uint32_t dictSize;
};

#endif
