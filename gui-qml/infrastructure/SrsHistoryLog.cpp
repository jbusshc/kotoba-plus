#include "SrsHistoryLog.h"
#include <cstdio>
#include <cstring>

/* ── Path ─────────────────────────────────────────────────────────────────── */

std::string SrsHistoryLog::chunkPath(uint32_t chunkIndex) const
{
    char suffix[32];
    snprintf(suffix, sizeof(suffix), ".history.%04u.log", chunkIndex);
    return m_basePath + suffix;
}

/* ── Ctor / Dtor ─────────────────────────────────────────────────────────── */

SrsHistoryLog::SrsHistoryLog(const std::string &basePath, uint32_t maxEventsPerChunk)
    : m_basePath(basePath), m_maxEventsPerChunk(maxEventsPerChunk)
{
    uint32_t latest = findLatestChunk();
    openOrCreateChunk(latest == 0 ? 1 : latest);
}

SrsHistoryLog::~SrsHistoryLog()
{
    if (m_file) { fflush(m_file); fclose(m_file); }
}

/* ── Chunk discovery ─────────────────────────────────────────────────────── */

uint32_t SrsHistoryLog::findLatestChunk() const
{
    uint32_t latest = 0;
    for (uint32_t i = 1; i <= 9999; ++i) {
        FILE *f = fopen(chunkPath(i).c_str(), "rb");
        if (!f) break;
        fclose(f);
        latest = i;
    }
    return latest;
}

/* ── Open / create chunk ─────────────────────────────────────────────────── */

void SrsHistoryLog::openOrCreateChunk(uint32_t index)
{
    if (m_file) { fflush(m_file); fclose(m_file); m_file = nullptr; }

    m_chunkIndex    = index;
    m_eventsInChunk = 0;

    std::string path = chunkPath(index);

    // Try existing file — count events already written
    FILE *existing = fopen(path.c_str(), "rb");
    if (existing) {
        HistoryChunkHeader hdr{};
        if (fread(&hdr, sizeof(hdr), 1, existing) == 1
            && hdr.magic   == HIST_MAGIC
            && hdr.version == HIST_VERSION)
        {
            fseek(existing, 0, SEEK_END);
            long data = ftell(existing) - (long)sizeof(HistoryChunkHeader);
            if (data > 0)
                m_eventsInChunk = (uint32_t)(data / sizeof(FsrsEvent));
        }
        fclose(existing);
        m_file = fopen(path.c_str(), "ab");
        return;
    }

    // New chunk — write header then reopen in append mode
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return;

    HistoryChunkHeader hdr{};
    hdr.magic       = HIST_MAGIC;
    hdr.version     = HIST_VERSION;
    hdr.chunk_index = index;
    hdr.reserved    = 0;
    fwrite(&hdr, sizeof(hdr), 1, f);
    fclose(f);

    m_file = fopen(path.c_str(), "ab");
}

/* ── Rotation ────────────────────────────────────────────────────────────── */

void SrsHistoryLog::rotateIfNeeded()
{
    if (m_maxEventsPerChunk > 0 && m_eventsInChunk >= m_maxEventsPerChunk)
        openOrCreateChunk(m_chunkIndex + 1);
}

/* ── Core append ─────────────────────────────────────────────────────────── */

void SrsHistoryLog::append(const FsrsEvent &ev)
{
    rotateIfNeeded();
    if (!m_file) return;
    fwrite(&ev, sizeof(ev), 1, m_file);
    fflush(m_file);
    ++m_eventsInChunk;
}

/* ── Event builder ───────────────────────────────────────────────────────── */

FsrsEvent SrsHistoryLog::makeEvent(fsrs_ev_type     type,
                                   uint32_t         entryId,
                                   const fsrs_card *card,
                                   fsrs_rating      rating,
                                   uint64_t         now) const
{
    FsrsEvent ev{};
    ev.device_id        = 0;          // history log doesn't need device identity
    ev.seq              = 0;          // not a sync event — no sequence number
    ev.timestamp        = now;
    ev.type             = static_cast<uint8_t>(type);
    ev.rating           = static_cast<uint8_t>(rating);
    ev.card_id          = entryId;

    if (card) {
        ev.card_due         = card->due_ts;
        ev.card_last_review = card->last_review;
        ev.card_stability   = card->stability;
        ev.card_difficulty  = card->difficulty;
        ev.card_reps        = card->reps;
        ev.card_lapses      = card->lapses;
        ev.card_state       = card->state;
        ev.card_step        = card->step;
        ev.card_flags       = card->flags;
    }

    return ev;
}

/* ── Public write API ────────────────────────────────────────────────────── */

void SrsHistoryLog::recordAdd(const fsrs_card *card, uint64_t now)
{
    if (!card) return;
    append(makeEvent(FSRS_EV_ADD, card->id, card, static_cast<fsrs_rating>(0), now));
}

void SrsHistoryLog::recordAnswer(const fsrs_card *card, fsrs_rating rating, uint64_t now)
{
    if (!card) return;
    append(makeEvent(FSRS_EV_REVIEW, card->id, card, rating, now));
}

void SrsHistoryLog::recordRemove(uint32_t entryId, const fsrs_card *before, uint64_t now)
{
    append(makeEvent(FSRS_EV_REMOVE, entryId, before, static_cast<fsrs_rating>(0), now));
}

void SrsHistoryLog::recordSuspend(const fsrs_card *before, uint64_t now)
{
    if (!before) return;
    append(makeEvent(FSRS_EV_SUSPEND, before->id, before, static_cast<fsrs_rating>(0), now));
}

void SrsHistoryLog::recordUnsuspend(const fsrs_card *after, uint64_t now)
{
    if (!after) return;
    // No dedicated unsuspend type — FSRS_EV_UPDATE carries full post-state
    append(makeEvent(FSRS_EV_UPDATE, after->id, after, static_cast<fsrs_rating>(0), now));
}

void SrsHistoryLog::recordUndo(uint32_t entryId, const fsrs_card *restored, uint64_t now)
{
    append(makeEvent(FSRS_EV_UNDO, entryId, restored, static_cast<fsrs_rating>(0), now));
}