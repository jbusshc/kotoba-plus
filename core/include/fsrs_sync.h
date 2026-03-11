#ifndef FSRS_SYNC_H
#define FSRS_SYNC_H

/*
 * fsrs_sync — Event-sourced sync layer for the FSRS-5 engine.
 *
 * Model
 * ─────
 * Every mutation (add, answer, remove, suspend) is recorded as an immutable
 * FsrsEvent.  Each event carries:
 *   - a vector-clock key  (device_id, seq, timestamp)  for ordering / dedup
 *   - the card id
 *   - the event type
 *   - for REVIEW / SUSPEND: the *resulting* card state snapshot
 *
 * Storing the result (not just the rating) is deliberate: FSRS stability
 * depends on elapsed time at the moment of answering, so replaying only a
 * rating would require knowing the exact deck state at that instant.  By
 * snapshotting the outcome we get cheap, deterministic rebuild with no
 * algorithm re-execution.
 *
 * Conflict resolution  (LWW per card)
 * ─────────────────────────────────────
 * Among all events for a given card, the one with the greatest
 * (timestamp, device_id, seq) wins.  Events are *always* kept in the log
 * for audit / undo; the LWW rule only determines which state is projected.
 *
 * Persistence layout
 * ───────────────────
 *   snapshot.bin  — compact card-state dump (fast cold start)
 *   events.log    — append-only stream of FsrsEvent structs
 *
 * Typical lifecycle:
 *   boot  : load snapshot → load log → apply log events (LWW merge)
 *   use   : every action appends one event to the log (fsrs_sync_log_append)
 *   sync  : exchange event logs with peer, call fsrs_sync_merge()
 *   nightly: fsrs_sync_compact() rewrites snapshot + clears log
 *
 * API integration
 * ────────────────
 *   After calling any fsrs_* mutating function, call the matching
 *   fsrs_sync_record_*() to capture the event.  The sync layer does NOT
 *   call into fsrs_deck itself — it is a pure observer / replayer.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fsrs.h"

/* ───────────────────────────────────────────────────────────────
 * Event types
 * ─────────────────────────────────────────────────────────────── */
typedef enum {
    FSRS_EV_ADD     = 0,   /* card created (new)               */
    FSRS_EV_REVIEW  = 1,   /* user answered                    */
    FSRS_EV_REMOVE  = 2,   /* card deleted                     */
    FSRS_EV_SUSPEND = 3,   /* card suspended / unsuspended     */
    FSRS_EV_UPDATE  = 4,   /* arbitrary field override         */
} fsrs_ev_type;

/* ───────────────────────────────────────────────────────────────
 * Event  (64 bytes, fixed-width, safe to fwrite/fread directly)
 *
 * Layout: all 8-byte fields first, then 4-byte, then smaller.
 *   [0..7]   device_id
 *   [8..15]  seq
 *   [16..23] timestamp
 *   [24..31] card_due
 *   [32..39] card_last_review
 *   [40..43] card_id
 *   [44..47] card_stability
 *   [48..51] card_difficulty
 *   [52..53] card_reps
 *   [54..55] card_lapses
 *   [56]     type
 *   [57]     rating
 *   [58]     card_state
 *   [59]     card_step
 *   [60]     card_flags
 *   [61..63] _pad
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    /* vector clock */
    uint64_t device_id;
    uint64_t seq;
    uint64_t timestamp;        /* unix seconds at time of action */

    /* card schedule (8-byte fields) */
    uint64_t card_due;
    uint64_t card_last_review;

    /* identity (4-byte) */
    uint32_t card_id;

    /* card memory model (4-byte) */
    float    card_stability;
    float    card_difficulty;

    /* counters (2-byte) */
    uint16_t card_reps;
    uint16_t card_lapses;

    /* event meta (1-byte) */
    uint8_t  type;             /* fsrs_ev_type                  */
    uint8_t  rating;           /* fsrs_rating (REVIEW only)     */
    uint8_t  card_state;       /* fsrs_state                    */
    uint8_t  card_step;
    uint8_t  card_flags;
    uint8_t  _pad[3];
} FsrsEvent;                   /* static_assert(sizeof == 64)   */

/* ───────────────────────────────────────────────────────────────
 * Sync context
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    uint64_t   device_id;
    uint64_t   next_seq;       /* monotonically increasing       */

    FsrsEvent *events;
    uint32_t   event_count;
    uint32_t   event_cap;
} FsrsSync;

/* ───────────────────────────────────────────────────────────────
 * Lifecycle
 * ─────────────────────────────────────────────────────────────── */
void fsrs_sync_init(FsrsSync *s, uint64_t device_id);
void fsrs_sync_free(FsrsSync *s);

/* ───────────────────────────────────────────────────────────────
 * Record local mutations
 * Call these AFTER the corresponding fsrs_* call succeeds.
 * Each function appends an event to s->events[] and returns a
 * pointer to it (valid until the next append / free).
 * ─────────────────────────────────────────────────────────────── */

/* card was just added via fsrs_add_card() */
FsrsEvent *fsrs_sync_record_add(FsrsSync *s,
                                const fsrs_card *card,
                                uint64_t now);

/* user just answered via fsrs_answer() */
FsrsEvent *fsrs_sync_record_answer(FsrsSync *s,
                                   const fsrs_card *card,
                                   fsrs_rating rating,
                                   uint64_t now);

/* card was just removed via fsrs_remove_card() */
FsrsEvent *fsrs_sync_record_remove(FsrsSync *s,
                                   uint32_t card_id,
                                   uint64_t now);

/* card was just suspended or unsuspended */
FsrsEvent *fsrs_sync_record_suspend(FsrsSync *s,
                                    const fsrs_card *card,
                                    uint64_t now);

/* ───────────────────────────────────────────────────────────────
 * Merge & rebuild
 * ─────────────────────────────────────────────────────────────── */

/* Merge remote events into the local log (dedup by device_id+seq).
 * Does NOT rebuild the deck — call fsrs_sync_rebuild() afterwards. */
void fsrs_sync_merge(FsrsSync *s,
                     const FsrsEvent *remote,
                     uint32_t remote_count);

/* Replay all events in the log (LWW per card) and apply the winning
 * state to the deck.  Rebuilds the deck's queues when done.
 * Cards not present in the deck are added automatically. */
void fsrs_sync_rebuild(FsrsSync *s, fsrs_deck *deck, uint64_t now);

/* ───────────────────────────────────────────────────────────────
 * Delta-sync helpers
 * ─────────────────────────────────────────────────────────────── */

/* Fill `out` (capacity `max`) with events from this device whose seq
 * is greater than `since_seq`.  Returns the number of events written.
 * Pass since_seq = 0 to get the full local history. */
uint32_t fsrs_sync_events_since(const FsrsSync *s,
                                uint64_t since_seq,
                                FsrsEvent *out,
                                uint32_t max);

/* Highest seq seen from a given device_id (useful for delta requests). */
uint64_t fsrs_sync_max_seq_for(const FsrsSync *s, uint64_t device_id);

/* ───────────────────────────────────────────────────────────────
 * Persistence
 * ─────────────────────────────────────────────────────────────── */

/* Append a single event to the log file (O(1), safe to call after
 * every fsrs_sync_record_*). */
bool fsrs_sync_log_append(const char *path, const FsrsEvent *ev);

/* Load all events from the log file, merging into s->events[]. */
bool fsrs_sync_log_load(FsrsSync *s, const char *path);

/* Snapshot: write the current projected card states (one per card in
 * the deck) so cold start avoids replaying the full event log. */
bool fsrs_sync_snapshot_save(const FsrsSync *s,
                             const fsrs_deck *deck,
                             const char *path);

/* Load snapshot into deck (card states only, not events). */
bool fsrs_sync_snapshot_load(fsrs_deck *deck,
                             const char *path,
                             uint64_t now);

/* Compact: save snapshot + truncate log + clear in-memory events.
 * Call periodically (e.g. after each session) to bound startup time. */
bool fsrs_sync_compact(FsrsSync *s,
                       fsrs_deck *deck,
                       const char *snapshot_path,
                       const char *log_path,
                       uint64_t now);

/* ───────────────────────────────────────────────────────────────
 * Utilities
 * ─────────────────────────────────────────────────────────────── */
static inline const char *fsrs_ev_type_str(fsrs_ev_type t) {
    switch (t) {
        case FSRS_EV_ADD:     return "ADD";
        case FSRS_EV_REVIEW:  return "REVIEW";
        case FSRS_EV_REMOVE:  return "REMOVE";
        case FSRS_EV_SUSPEND: return "SUSPEND";
        case FSRS_EV_UPDATE:  return "UPDATE";
    }
    return "?";
}

#ifdef __cplusplus
}
#endif

#endif /* FSRS_SYNC_H */