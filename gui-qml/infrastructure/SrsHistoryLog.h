#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include <QFile>

extern "C" {
#include "../../core/include/fsrs.h"
#include "../../core/include/fsrs_sync.h"
}

/*
 * SrsHistoryLog — append-only historical log using FsrsEvent natively.
 *
 * Android compatibility: uses QFile instead of raw FILE* so that paths in
 * AppDataLocation (internal storage) are opened correctly on all platforms.
 *
 * File naming: <basePath>.history.0001.log, .0002.log, ...
 *
 * File layout per chunk:
 *   [HistoryChunkHeader 16 bytes]
 *   [FsrsEvent 64 bytes] × N
 */

struct __attribute__((packed)) HistoryChunkHeader {
    uint32_t magic;        // HIST_MAGIC = 0x48535253 ("SRSH")
    uint32_t version;      // HIST_VERSION = 1
    uint32_t chunk_index;  // 1-based
    uint32_t reserved;     // 0
};

static_assert(sizeof(HistoryChunkHeader) == 16, "HistoryChunkHeader size mismatch");

class SrsHistoryLog {
public:
    static constexpr uint32_t HIST_MAGIC           = 0x48535253u;
    static constexpr uint32_t HIST_VERSION         = 1u;
    static constexpr uint32_t DEFAULT_CHUNK_EVENTS = 1000;

    explicit SrsHistoryLog(const std::string &basePath,
                           uint32_t maxEventsPerChunk = DEFAULT_CHUNK_EVENTS);
    ~SrsHistoryLog();

    SrsHistoryLog(const SrsHistoryLog &) = delete;
    SrsHistoryLog &operator=(const SrsHistoryLog &) = delete;

    void recordAdd       (const fsrs_card *card,    uint64_t now);
    void recordAnswer    (const fsrs_card *card,    fsrs_rating rating, uint64_t now);
    void recordRemove    (uint32_t entryId, const fsrs_card *before, uint64_t now);
    void recordSuspend   (const fsrs_card *before,  uint64_t now);
    void recordUnsuspend (const fsrs_card *after,   uint64_t now);
    void recordUndo      (uint32_t entryId, const fsrs_card *restored, uint64_t now);

    uint32_t    currentChunk()         const { return m_chunkIndex; }
    uint32_t    eventsInCurrentChunk() const { return m_eventsInChunk; }
    std::string chunkPath(uint32_t chunkIndex) const;

private:
    void     append(const FsrsEvent &ev);
    void     openOrCreateChunk(uint32_t index);
    void     rotateIfNeeded();
    uint32_t findLatestChunk() const;

    FsrsEvent makeEvent(fsrs_ev_type    type,
                        uint32_t        entryId,
                        const fsrs_card *card,
                        fsrs_rating     rating,
                        uint64_t        now) const;

    std::string              m_basePath;
    uint32_t                 m_maxEventsPerChunk;
    uint32_t                 m_chunkIndex    = 0;
    uint32_t                 m_eventsInChunk = 0;
    std::unique_ptr<QFile>   m_file;           // replaces raw FILE*
};
