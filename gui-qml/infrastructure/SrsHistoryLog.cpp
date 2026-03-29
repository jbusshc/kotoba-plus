#include "SrsHistoryLog.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
// SrsHistoryLog — Android-compatible rewrite.
//
// The original used raw FILE* (fopen/fclose/fwrite). On Android, fopen()
// works fine for paths in internal storage (AppDataLocation), but Qt's
// QFile is safer because it handles path encoding, permissions, and Android
// scoped storage transparently.
//
// All I/O is now done via QFile. The binary layout of the chunk files is
// unchanged (HistoryChunkHeader + FsrsEvent records), so existing logs
// remain readable.
// ─────────────────────────────────────────────────────────────────────────────

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
    if (m_file && m_file->isOpen()) {
        m_file->flush();
        m_file->close();
    }
}

/* ── Chunk discovery ─────────────────────────────────────────────────────── */

uint32_t SrsHistoryLog::findLatestChunk() const
{
    uint32_t latest = 0;
    for (uint32_t i = 1; i <= 9999; ++i) {
        if (!QFileInfo::exists(QString::fromStdString(chunkPath(i)))) break;
        latest = i;
    }
    return latest;
}

/* ── Open / create chunk ─────────────────────────────────────────────────── */

void SrsHistoryLog::openOrCreateChunk(uint32_t index)
{
    if (m_file && m_file->isOpen()) {
        m_file->flush();
        m_file->close();
    }
    m_file.reset();

    m_chunkIndex    = index;
    m_eventsInChunk = 0;

    QString path = QString::fromStdString(chunkPath(index));

    if (QFileInfo::exists(path)) {
        // Count events already written in existing chunk
        QFile reader(path);
        if (reader.open(QIODevice::ReadOnly)) {
            HistoryChunkHeader hdr{};
            if (reader.read(reinterpret_cast<char*>(&hdr), sizeof(hdr)) == sizeof(hdr)
                && hdr.magic   == HIST_MAGIC
                && hdr.version == HIST_VERSION)
            {
                qint64 dataBytes = reader.size() - static_cast<qint64>(sizeof(HistoryChunkHeader));
                if (dataBytes > 0)
                    m_eventsInChunk = static_cast<uint32_t>(dataBytes / sizeof(FsrsEvent));
            }
            reader.close();
        }

        // Open in append mode
        m_file = std::make_unique<QFile>(path);
        if (!m_file->open(QIODevice::Append)) {
            qWarning() << "SrsHistoryLog: failed to open chunk for append:" << path;
            m_file.reset();
        }
        return;
    }

    // New chunk — write header then reopen for append
    {
        QFile writer(path);
        if (!writer.open(QIODevice::WriteOnly)) {
            qWarning() << "SrsHistoryLog: failed to create chunk:" << path;
            return;
        }
        HistoryChunkHeader hdr{};
        hdr.magic       = HIST_MAGIC;
        hdr.version     = HIST_VERSION;
        hdr.chunk_index = index;
        hdr.reserved    = 0;
        writer.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        writer.close();
    }

    m_file = std::make_unique<QFile>(path);
    if (!m_file->open(QIODevice::Append)) {
        qWarning() << "SrsHistoryLog: failed to open new chunk for append:" << path;
        m_file.reset();
    }
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
    if (!m_file || !m_file->isOpen()) return;
    m_file->write(reinterpret_cast<const char*>(&ev), sizeof(ev));
    m_file->flush();
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
    ev.device_id        = 0;
    ev.seq              = 0;
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
    append(makeEvent(FSRS_EV_UPDATE, after->id, after, static_cast<fsrs_rating>(0), now));
}

void SrsHistoryLog::recordUndo(uint32_t entryId, const fsrs_card *restored, uint64_t now)
{
    append(makeEvent(FSRS_EV_UNDO, entryId, restored, static_cast<fsrs_rating>(0), now));
}
