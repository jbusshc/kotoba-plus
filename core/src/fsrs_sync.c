/*
 * fsrs_sync.c — event-sourced sync layer for the FSRS-5 engine.
 */

#include "fsrs_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* ─────────────────────────────────────────────────────────────── *
 * Compile-time sanity check
 * ─────────────────────────────────────────────────────────────── */
typedef char _fsrs_event_size_check
    [(sizeof(FsrsEvent) == 64) ? 1 : -1];

/* ─────────────────────────────────────────────────────────────── *
 * File magic / version
 * ─────────────────────────────────────────────────────────────── */
#define SYNC_LOG_MAGIC      0x5352464C  /* "FSRL" */
#define SYNC_SNAP_MAGIC     0x53524653  /* "FSRS" */
#define SYNC_LOG_VERSION    1u
#define SYNC_SNAP_VERSION   1u

/* ─────────────────────────────────────────────────────────────── *
 * Internal helpers
 * ─────────────────────────────────────────────────────────────── */

static bool _ev_grow(FsrsSync *s) {
    if (s->event_count < s->event_cap) return true;
    uint32_t nc = s->event_cap ? s->event_cap * 2 : 64;
    FsrsEvent *nb = realloc(s->events, nc * sizeof(FsrsEvent));
    if (!nb) return false;
    s->events = nb;
    s->event_cap = nc;
    return true;
}

/* Copy card fields into an event snapshot */
static void _snap_card(FsrsEvent *ev, const fsrs_card *c) {
    ev->card_state       = c->state;
    ev->card_step        = c->step;
    ev->card_flags       = c->flags;
    ev->card_due         = c->due;
    ev->card_last_review = c->last_review;
    ev->card_stability   = c->stability;
    ev->card_difficulty  = c->difficulty;
    ev->card_reps        = c->reps;
    ev->card_lapses      = c->lapses;
}

/* Fill the common vector-clock fields of a new local event */
static FsrsEvent *_new_event(FsrsSync *s, uint32_t card_id,
                              fsrs_ev_type type, uint64_t now) {
    if (!_ev_grow(s)) return NULL;
    FsrsEvent *ev = &s->events[s->event_count++];
    memset(ev, 0, sizeof(*ev));
    ev->device_id = s->device_id;
    ev->seq       = s->next_seq++;
    ev->timestamp = now;
    ev->card_id   = card_id;
    ev->type      = (uint8_t)type;
    return ev;
}

/* LWW comparator: returns true if `a` beats `b` */
static bool _lww_wins(const FsrsEvent *a, const FsrsEvent *b) {
    if (a->timestamp != b->timestamp)
        return a->timestamp > b->timestamp;
    if (a->device_id != b->device_id)
        return a->device_id > b->device_id;
    return a->seq > b->seq;
}

/* Used by qsort for deterministic replay order */
static int _ev_cmp(const void *pa, const void *pb) {
    const FsrsEvent *a = (const FsrsEvent *)pa;
    const FsrsEvent *b = (const FsrsEvent *)pb;
    if (a->timestamp != b->timestamp)
        return (a->timestamp < b->timestamp) ? -1 : 1;
    if (a->device_id != b->device_id)
        return (a->device_id < b->device_id) ? -1 : 1;
    if (a->seq != b->seq)
        return (a->seq < b->seq) ? -1 : 1;
    return 0;
}

/* ─────────────────────────────────────────────────────────────── *
 * Lifecycle
 * ─────────────────────────────────────────────────────────────── */

void fsrs_sync_init(FsrsSync *s, uint64_t device_id) {
    memset(s, 0, sizeof(*s));
    s->device_id = device_id;
    s->next_seq  = 1;
}

void fsrs_sync_free(FsrsSync *s) {
    free(s->events);
    memset(s, 0, sizeof(*s));
}

/* ─────────────────────────────────────────────────────────────── *
 * Record local mutations
 * ─────────────────────────────────────────────────────────────── */

FsrsEvent *fsrs_sync_record_add(FsrsSync *s,
                                const fsrs_card *card,
                                uint64_t now) {
    FsrsEvent *ev = _new_event(s, card->id, FSRS_EV_ADD, now);
    if (!ev) return NULL;
    _snap_card(ev, card);
    return ev;
}

FsrsEvent *fsrs_sync_record_answer(FsrsSync *s,
                                   const fsrs_card *card,
                                   fsrs_rating rating,
                                   uint64_t now) {
    FsrsEvent *ev = _new_event(s, card->id, FSRS_EV_REVIEW, now);
    if (!ev) return NULL;
    ev->rating = (uint8_t)rating;
    _snap_card(ev, card);
    return ev;
}

FsrsEvent *fsrs_sync_record_remove(FsrsSync *s,
                                   uint32_t card_id,
                                   uint64_t now) {
    FsrsEvent *ev = _new_event(s, card_id, FSRS_EV_REMOVE, now);
    return ev; /* no card snapshot: the card is gone */
}

FsrsEvent *fsrs_sync_record_suspend(FsrsSync *s,
                                    const fsrs_card *card,
                                    uint64_t now) {
    FsrsEvent *ev = _new_event(s, card->id, FSRS_EV_SUSPEND, now);
    if (!ev) return NULL;
    _snap_card(ev, card);
    return ev;
}

/* ─────────────────────────────────────────────────────────────── *
 * Merge remote events
 * ─────────────────────────────────────────────────────────────── */

void fsrs_sync_merge(FsrsSync *s,
                     const FsrsEvent *remote,
                     uint32_t remote_count) {
    for (uint32_t i = 0; i < remote_count; i++) {
        const FsrsEvent *ev = &remote[i];

        /* Dedup: skip if we already have (device_id, seq) */
        bool found = false;
        for (uint32_t j = 0; j < s->event_count; j++) {
            if (s->events[j].device_id == ev->device_id &&
                s->events[j].seq       == ev->seq) {
                found = true;
                break;
            }
        }
        if (found) continue;

        if (!_ev_grow(s)) return; /* OOM — stop, don't corrupt */
        s->events[s->event_count++] = *ev;
    }

    /* Keep next_seq ahead of all known local seqs */
    for (uint32_t i = 0; i < s->event_count; i++) {
        if (s->events[i].device_id == s->device_id &&
            s->events[i].seq >= s->next_seq) {
            s->next_seq = s->events[i].seq + 1;
        }
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * Rebuild deck from event log  (LWW per card)
 *
 * Strategy:
 *   1. Sort events by (timestamp, device_id, seq)  — deterministic order.
 *   2. For each card, find the single winning event via LWW.
 *   3. Apply that event's card-state snapshot to the deck.
 *
 * Cards in the deck that have no events are left untouched.
 * Cards with a REMOVE event as winner are removed from the deck.
 * Cards with ADD/REVIEW/SUSPEND as winner are upserted into the deck.
 * ─────────────────────────────────────────────────────────────── */

void fsrs_sync_rebuild(FsrsSync *s, fsrs_deck *deck, uint64_t now) {
    if (!s->event_count) {
        fsrs_deck_rebuild_queues(deck, now);
        return;
    }

    /* Sort events for deterministic processing */
    qsort(s->events, s->event_count, sizeof(FsrsEvent), _ev_cmp);

    /* ── Pass 1: find the LWW-winning event per card_id ── */

    /* We need a temporary map card_id → winning event index.
     * Since card ids may be sparse, we scan linearly — fine for
     * typical deck sizes (< 100k cards). */

    /* Collect unique card ids first */
    uint32_t *ids      = NULL;
    uint32_t  id_count = 0;
    uint32_t  id_cap   = 0;

    for (uint32_t i = 0; i < s->event_count; i++) {
        uint32_t cid = s->events[i].card_id;
        bool found = false;
        for (uint32_t j = 0; j < id_count; j++) {
            if (ids[j] == cid) { found = true; break; }
        }
        if (!found) {
            if (id_count == id_cap) {
                id_cap = id_cap ? id_cap * 2 : 32;
                uint32_t *nb = realloc(ids, id_cap * sizeof(uint32_t));
                if (!nb) { free(ids); return; }
                ids = nb;
            }
            ids[id_count++] = cid;
        }
    }

    /* For each unique card, find the winning event */
    for (uint32_t ci = 0; ci < id_count; ci++) {
        uint32_t cid = ids[ci];

        const FsrsEvent *winner = NULL;
        for (uint32_t i = 0; i < s->event_count; i++) {
            const FsrsEvent *ev = &s->events[i];
            if (ev->card_id != cid) continue;
            if (!winner || _lww_wins(ev, winner))
                winner = ev;
        }
        if (!winner) continue;

        /* ── Apply winner to deck ── */

        if (winner->type == FSRS_EV_REMOVE) {
            /* Remove from deck if present */
            if (fsrs_has_card(deck, cid))
                fsrs_remove_card(deck, cid);
            continue;
        }

        /* Upsert: add card if missing */
        if (!fsrs_has_card(deck, cid)) {
            fsrs_card *nc = fsrs_add_card(deck, cid, winner->timestamp);
            if (!nc) continue; /* OOM */
        }

        fsrs_card *card = fsrs_get_card(deck, cid);
        if (!card) continue;

        /* Overwrite card state from the event snapshot */
        card->state       = winner->card_state;
        card->step        = winner->card_step;
        card->flags       = winner->card_flags;
        card->due         = winner->card_due;
        card->last_review = winner->card_last_review;
        card->stability   = winner->card_stability;
        card->difficulty  = winner->card_difficulty;
        card->reps        = winner->card_reps;
        card->lapses      = winner->card_lapses;
    }

    free(ids);

    /* Rebuild queues so the deck is ready to serve cards */
    fsrs_deck_rebuild_queues(deck, now);
}

/* ─────────────────────────────────────────────────────────────── *
 * Delta-sync helpers
 * ─────────────────────────────────────────────────────────────── */

uint32_t fsrs_sync_events_since(const FsrsSync *s,
                                uint64_t since_seq,
                                FsrsEvent *out,
                                uint32_t max) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->event_count && n < max; i++) {
        if (s->events[i].device_id == s->device_id &&
            s->events[i].seq > since_seq) {
            out[n++] = s->events[i];
        }
    }
    return n;
}

uint64_t fsrs_sync_max_seq_for(const FsrsSync *s, uint64_t device_id) {
    uint64_t max = 0;
    for (uint32_t i = 0; i < s->event_count; i++) {
        if (s->events[i].device_id == device_id &&
            s->events[i].seq > max) {
            max = s->events[i].seq;
        }
    }
    return max;
}

/* ─────────────────────────────────────────────────────────────── *
 * Log persistence  (append-only stream of FsrsEvent)
 *
 * File format:
 *   [4]  magic   = SYNC_LOG_MAGIC
 *   [4]  version = SYNC_LOG_VERSION
 *   [8]  device_id  (of the writer — informational)
 *   then: N × sizeof(FsrsEvent)  (no count prefix; derive from file size)
 *
 * The header is only written by fsrs_sync_log_append when the file
 * doesn't exist yet (size == 0).  Subsequent appends go straight to
 * the struct stream.
 * ─────────────────────────────────────────────────────────────── */

#define LOG_HEADER_SIZE  (4 + 4 + 8)   /* magic + version + device_id */

bool fsrs_sync_log_append(const char *path, const FsrsEvent *ev) {
    /* Open for append — creates the file if missing */
    FILE *f = fopen(path, "ab");
    if (!f) return false;

    /* If file is empty, write the header first */
    fseek(f, 0, SEEK_END);
    long pos = ftell(f);
    if (pos == 0) {
        uint32_t magic   = SYNC_LOG_MAGIC;
        uint32_t version = SYNC_LOG_VERSION;
        bool ok =
            fwrite(&magic,         4, 1, f) == 1 &&
            fwrite(&version,       4, 1, f) == 1 &&
            fwrite(&ev->device_id, 8, 1, f) == 1;
        if (!ok) { fclose(f); return false; }
    }

    bool ok = fwrite(ev, sizeof(FsrsEvent), 1, f) == 1;
    fclose(f);
    return ok;
}

bool fsrs_sync_log_load(FsrsSync *s, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return true; /* missing log is fine — empty */

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size == 0) { fclose(f); return true; }

    /* Read and validate header */
    uint32_t magic, version;
    uint64_t writer_device;

    if (fread(&magic,         4, 1, f) != 1 || magic   != SYNC_LOG_MAGIC ||
        fread(&version,       4, 1, f) != 1 || version != SYNC_LOG_VERSION ||
        fread(&writer_device, 8, 1, f) != 1) {
        fclose(f);
        return false;
    }

    long data_bytes = size - LOG_HEADER_SIZE;
    if (data_bytes < 0 || data_bytes % (long)sizeof(FsrsEvent) != 0) {
        /* Truncated or corrupted — load what we can */
        data_bytes -= data_bytes % (long)sizeof(FsrsEvent);
    }

    uint32_t count = (uint32_t)(data_bytes / sizeof(FsrsEvent));
    if (count == 0) { fclose(f); return true; }

    FsrsEvent *tmp = malloc(count * sizeof(FsrsEvent));
    if (!tmp) { fclose(f); return false; }

    uint32_t got = (uint32_t)fread(tmp, sizeof(FsrsEvent), count, f);
    fclose(f);

    /* Merge into sync (dedup) */
    fsrs_sync_merge(s, tmp, got);
    free(tmp);
    return true;
}

/* ─────────────────────────────────────────────────────────────── *
 * Snapshot persistence  (current projected card states)
 *
 * File format:
 *   [4]  magic   = SYNC_SNAP_MAGIC
 *   [4]  version = SYNC_SNAP_VERSION
 *   [8]  device_id
 *   [8]  next_seq
 *   [4]  card_count
 *   then: card_count × snapshot_record
 *
 * snapshot_record (44 bytes):
 *   [4]  card_id
 *   [1]  state
 *   [1]  step
 *   [1]  flags
 *   [1]  _pad
 *   [8]  due
 *   [8]  last_review
 *   [4]  stability
 *   [4]  difficulty
 *   [2]  reps
 *   [2]  lapses
 *   [8]  winning_timestamp  (for later conflict resolution if needed)
 * ─────────────────────────────────────────────────────────────── */

typedef struct {
    uint32_t card_id;
    uint8_t  state;
    uint8_t  step;
    uint8_t  flags;
    uint8_t  _pad;
    uint64_t due;
    uint64_t last_review;
    float    stability;
    float    difficulty;
    uint16_t reps;
    uint16_t lapses;
    uint64_t winning_timestamp;  /* LWW timestamp of the state that won */
} _snap_record;                  /* 44 bytes */

bool fsrs_sync_snapshot_save(const FsrsSync *s,
                             const fsrs_deck *deck,
                             const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint32_t magic   = SYNC_SNAP_MAGIC;
    uint32_t version = SYNC_SNAP_VERSION;
    uint32_t count   = deck->count;

    bool ok =
        fwrite(&magic,         4, 1, f) == 1 &&
        fwrite(&version,       4, 1, f) == 1 &&
        fwrite(&s->device_id,  8, 1, f) == 1 &&
        fwrite(&s->next_seq,   8, 1, f) == 1 &&
        fwrite(&count,         4, 1, f) == 1;

    if (!ok) { fclose(f); return false; }

    for (uint32_t i = 0; i < deck->count && ok; i++) {
        const fsrs_card *c = &deck->cards[i];

        /* Find the winning timestamp for this card from the event log */
        uint64_t win_ts = 0;
        for (uint32_t j = 0; j < s->event_count; j++) {
            if (s->events[j].card_id == c->id &&
                s->events[j].timestamp > win_ts)
                win_ts = s->events[j].timestamp;
        }

        _snap_record rec = {
            .card_id           = c->id,
            .state             = c->state,
            .step              = c->step,
            .flags             = c->flags,
            ._pad              = 0,
            .due               = c->due,
            .last_review       = c->last_review,
            .stability         = c->stability,
            .difficulty        = c->difficulty,
            .reps              = c->reps,
            .lapses            = c->lapses,
            .winning_timestamp = win_ts,
        };
        ok = fwrite(&rec, sizeof(rec), 1, f) == 1;
    }

    fclose(f);
    return ok;
}

bool fsrs_sync_snapshot_load(fsrs_deck *deck,
                             const char *path,
                             uint64_t now) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint32_t magic, version, count;
    uint64_t device_id, next_seq;

    bool ok =
        fread(&magic,     4, 1, f) == 1 && magic   == SYNC_SNAP_MAGIC &&
        fread(&version,   4, 1, f) == 1 && version == SYNC_SNAP_VERSION &&
        fread(&device_id, 8, 1, f) == 1 &&
        fread(&next_seq,  8, 1, f) == 1 &&
        fread(&count,     4, 1, f) == 1;

    if (!ok) { fclose(f); return false; }

    for (uint32_t i = 0; i < count && ok; i++) {
        _snap_record rec;
        if (fread(&rec, sizeof(rec), 1, f) != 1) { ok = false; break; }

        /* Upsert card into deck */
        if (!fsrs_has_card(deck, rec.card_id)) {
            fsrs_card *nc = fsrs_add_card(deck, rec.card_id, rec.due);
            if (!nc) { ok = false; break; }
        }

        fsrs_card *c = fsrs_get_card(deck, rec.card_id);
        if (!c) { ok = false; break; }

        c->state       = rec.state;
        c->step        = rec.step;
        c->flags       = rec.flags;
        c->due         = rec.due;
        c->last_review = rec.last_review;
        c->stability   = rec.stability;
        c->difficulty  = rec.difficulty;
        c->reps        = rec.reps;
        c->lapses      = rec.lapses;
    }

    fclose(f);
    if (ok) fsrs_deck_rebuild_queues(deck, now);
    return ok;
}

/* ─────────────────────────────────────────────────────────────── *
 * Compact: snapshot + clear log + clear in-memory events
 * ─────────────────────────────────────────────────────────────── */

bool fsrs_sync_compact(FsrsSync *s,
                       fsrs_deck *deck,
                       const char *snapshot_path,
                       const char *log_path,
                       uint64_t now) {
    /* 1. Project current state */
    fsrs_sync_rebuild(s, deck, now);

    /* 2. Save snapshot */
    if (!fsrs_sync_snapshot_save(s, deck, snapshot_path))
        return false;

    /* 3. Truncate log file */
    FILE *f = fopen(log_path, "wb");
    if (!f) return false;
    fclose(f);

    /* 4. Clear in-memory event log */
    free(s->events);
    s->events      = NULL;
    s->event_count = 0;
    s->event_cap   = 0;

    return true;
}