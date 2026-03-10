#pragma once
#include <optional>
#include <cstdint>
#include <vector>

extern "C" {
#include <srs.h>
#include <srs_sync.h>
}

class Configuration;

class SrsService
{
public:
    explicit SrsService(uint32_t dictSize, Configuration *config);
    ~SrsService();

    bool load(const char *path);
    bool save(const char *path);

    bool add(uint32_t entryId);
    bool remove(uint32_t entryId);
    bool contains(uint32_t entryId) const;

    std::optional<srs_review> nextDue();
    void answer(uint32_t entryId, srs_quality q);

    uint32_t dueCount(uint64_t now) const;
    uint32_t learningCount() const;
    uint32_t newCount() const;
    uint32_t lapsedCount() const;

private:

    int32_t indexOf(uint32_t entryId) const;

    srs_profile m_profile;
    uint32_t m_dictSize;
    Configuration *m_config;

    SrsSync m_sync;

    // entryId → index in heap
    std::vector<int32_t> m_idIndex;
};