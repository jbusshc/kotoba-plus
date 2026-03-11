#ifndef FSRS_H
#define FSRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════
 * FSRS-5 Spaced Repetition System
 * Pure C implementation, no dependencies.
 *
 * Implements FSRS-5 algorithm:
 *   https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm
 *
 * Design goals:
 *   - Minimal allocations (flat arrays, no linked lists)
 *   - Min-heaps for O(log n) scheduling
 *   - Bitmap for O(1) membership
 *   - Deterministic given same seed
 * ═══════════════════════════════════════════════════════════════ */

/* ───────────────────────────────────────────────────────────────
 * Version
 * ─────────────────────────────────────────────────────────────── */
#define FSRS_VERSION_MAJOR 1
#define FSRS_VERSION_MINOR 0

/* ───────────────────────────────────────────────────────────────
 * Time utilities
 *   "Day" boundary = 04:00 local (configurable via fsrs_set_day_offset)
 * ─────────────────────────────────────────────────────────────── */
#define FSRS_SEC_PER_DAY  86400ULL

/* Returns current unix timestamp */
static inline uint64_t fsrs_now(void) {
    return (uint64_t)time(NULL);
}

/* ───────────────────────────────────────────────────────────────
 * Rating (what the user presses)
 * ─────────────────────────────────────────────────────────────── */
typedef enum {
    FSRS_AGAIN = 1,   /* complete blackout / forgot */
    FSRS_HARD  = 2,   /* recalled with serious difficulty */
    FSRS_GOOD  = 3,   /* recalled after a hesitation */
    FSRS_EASY  = 4    /* recalled perfectly, felt easy */
} fsrs_rating;

/* ───────────────────────────────────────────────────────────────
 * Card state machine
 *
 *   NEW ──────────────────────► LEARNING
 *   LEARNING ──(graduated)────► REVIEW
 *   REVIEW   ──(lapsed)───────► RELEARNING
 *   RELEARNING ──(graduated)──► REVIEW
 *   any ──(suspend)───────────► SUSPENDED
 * ─────────────────────────────────────────────────────────────── */
typedef enum {
    FSRS_STATE_NEW        = 0,
    FSRS_STATE_LEARNING   = 1,
    FSRS_STATE_REVIEW     = 2,
    FSRS_STATE_RELEARNING = 3,
    FSRS_STATE_SUSPENDED  = 4
} fsrs_state;

/* ───────────────────────────────────────────────────────────────
 * Card flags (bitmask)
 * ─────────────────────────────────────────────────────────────── */
#define FSRS_FLAG_LEECH   0x01u   /* too many lapses */
#define FSRS_FLAG_BURIED  0x02u   /* skip until tomorrow */

/* ───────────────────────────────────────────────────────────────
 * Card (persistent, serialisable)
 *
 * The struct is 64-byte aligned so it packs tightly in arrays.
 * Fields labelled [FSRS] are owned by the algorithm.
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    /* identity */
    uint32_t id;          /* user-assigned card identifier */

    /* state */
    uint8_t  state;       /* fsrs_state */
    uint8_t  step;        /* current learning/relearning step index */
    uint8_t  flags;       /* FSRS_FLAG_* bitmask */
    uint8_t  _pad0;

    /* scheduling */
    uint64_t due;         /* unix timestamp: when to show next */
    uint64_t last_review; /* unix timestamp of last review (0 = never) */

    /* counters */
    uint16_t reps;        /* total successful reviews */
    uint16_t lapses;      /* times forgotten from REVIEW state */
    uint32_t _pad1;

    /* [FSRS] memory model */
    float stability;      /* S: days until 90% retention */
    float difficulty;     /* D: inherent difficulty [1..10] */

    /* computed, not stored (derived from stability + elapsed days) */
    /* float retrievability; */
} fsrs_card;

/* ───────────────────────────────────────────────────────────────
 * FSRS-5 parameters (19 weights + 4 scalars)
 * Default values are from the FSRS-5 paper.
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    /* w[0..18]: FSRS-5 weights */
    float w[19];

    /* Desired retention (0..1). Default: 0.9 */
    float desired_retention;

    /* Maximum interval in days. Default: 36500 (100 years) */
    uint32_t maximum_interval;

    /* Leech threshold: auto-suspend after N lapses. 0 = disabled. */
    uint16_t leech_threshold;

    /* Learning steps in seconds, e.g. {60, 600} = 1min then 10min */
    uint32_t *learning_steps;
    uint32_t  learning_steps_count;

    /* Relearning steps in seconds, e.g. {600} = 10min */
    uint32_t *relearning_steps;
    uint32_t  relearning_steps_count;

    /* Day boundary offset from midnight in seconds. Default: 4*3600 = 04:00 */
    uint32_t day_offset_secs;

    /* New cards per day limit. 0 = unlimited. */
    uint32_t new_cards_per_day;

    /* Reviews per day limit. 0 = unlimited. */
    uint32_t reviews_per_day;
} fsrs_params;

/* Returns a params struct with sensible FSRS-5 defaults.
 * Caller must set learning_steps / relearning_steps pointers
 * (or leave NULL for built-in defaults). */
fsrs_params fsrs_default_params(void);

/* ───────────────────────────────────────────────────────────────
 * Deck  — the main container
 *
 * Owns all memory. Use fsrs_deck_init / fsrs_deck_free.
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    /* card storage */
    fsrs_card  *cards;
    uint32_t    count;
    uint32_t    capacity;

    /* id→index lookup  (bitmap for containment, hash-map for index) */
    uint8_t    *bitmap;          /* bit[id] = 1 if card exists */
    uint32_t    bitmap_bits;     /* number of bits in bitmap */
    uint32_t   *id_to_index;     /* id_to_index[id] = index in cards[] */
    uint32_t    id_map_size;     /* allocated slots */

    /* queues (store indices into cards[]) */
    uint32_t   *due_heap;        /* min-heap by due time: REVIEW cards */
    uint32_t    due_heap_size;
    uint32_t    due_heap_cap;

    uint32_t   *learn_heap;      /* min-heap by due time: LEARNING + RELEARNING */
    uint32_t    learn_heap_size;
    uint32_t    learn_heap_cap;

    uint32_t   *new_queue;       /* NEW cards (indices), array */
    uint32_t    new_queue_size;
    uint32_t    new_queue_cap;

    /* daily counters (reset each logical day) */
    uint64_t    current_day;     /* logical day index */
    uint32_t    new_today;
    uint32_t    reviews_today;

    /* configuration */
    fsrs_params params;

    /* internal learning steps (may point into params or built-in defaults) */
    uint32_t   *_learn_steps;
    uint32_t    _learn_steps_n;
    uint32_t   *_relearn_steps;
    uint32_t    _relearn_steps_n;
} fsrs_deck;

/* ───────────────────────────────────────────────────────────────
 * Deck lifecycle
 * ─────────────────────────────────────────────────────────────── */

/* Initialise a deck.
 *   id_space: maximum card id you'll ever use (for bitmap / id_map sizing).
 *   params:   NULL → use fsrs_default_params().
 * Returns true on success. */
bool fsrs_deck_init(fsrs_deck *d, uint32_t id_space, const fsrs_params *params);

/* Release all memory owned by the deck. */
void fsrs_deck_free(fsrs_deck *d);

/* Rebuild all queues from scratch (call after bulk load). */
void fsrs_deck_rebuild_queues(fsrs_deck *d, uint64_t now);

/* ───────────────────────────────────────────────────────────────
 * Card management
 * ─────────────────────────────────────────────────────────────── */

/* Add a new card with the given id.
 * Returns pointer to the card on success, NULL if already exists or OOM. */
fsrs_card *fsrs_add_card(fsrs_deck *d, uint32_t id, uint64_t now);

/* Remove a card by id. Returns false if not found. */
bool fsrs_remove_card(fsrs_deck *d, uint32_t id);

/* Look up a card by id. Returns NULL if not found. */
fsrs_card *fsrs_get_card(fsrs_deck *d, uint32_t id);

/* Quick membership test. */
static inline bool fsrs_has_card(const fsrs_deck *d, uint32_t id) {
    if (id >= d->bitmap_bits) return false;
    return (d->bitmap[id >> 3] >> (id & 7)) & 1u;
}

/* Suspend / unsuspend a card. */
void fsrs_suspend(fsrs_deck *d, uint32_t id);
void fsrs_unsuspend(fsrs_deck *d, uint32_t id, uint64_t now);

/* ───────────────────────────────────────────────────────────────
 * Review session
 * ─────────────────────────────────────────────────────────────── */

/* Return the next card to review (does NOT remove from queue).
 * Priority: learn_heap due → review due today → new (if limit allows)
 *           → learn_heap not-yet-due (if nothing else left in session).
 * Returns NULL if nothing is due today and no learning cards pending. */
fsrs_card *fsrs_next_card(fsrs_deck *d, uint64_t now);

/* Seconds until the next card becomes showable (0 = one is ready now).
 * Returns UINT64_MAX if there is nothing left for today. */
uint64_t fsrs_wait_seconds(fsrs_deck *d, uint64_t now);

/* Record the user's answer and reschedule the card.
 * This is the main entry point — it updates all FSRS fields
 * and re-inserts the card into the correct queue. */
void fsrs_answer(fsrs_deck *d, fsrs_card *card, fsrs_rating rating, uint64_t now);

/* Preview: compute what `due` would be for each rating,
 * without modifying the card. out[4] receives values for
 * AGAIN/HARD/GOOD/EASY (indices 0..3). */
void fsrs_preview(const fsrs_deck *d, const fsrs_card *card,
                  uint64_t now, uint64_t out[4]);

/* ───────────────────────────────────────────────────────────────
 * Statistics
 * ─────────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t total;
    uint32_t new_count;
    uint32_t learning_count;
    uint32_t relearning_count;
    uint32_t review_count;
    uint32_t suspended_count;
    uint32_t leech_count;
    uint32_t due_now;          /* cards due at `now` */
    uint32_t due_today;        /* cards due before end of logical day */
    float    average_stability;
    float    average_difficulty;
    float    retention;        /* estimated average retrievability */
} fsrs_stats;

void fsrs_compute_stats(const fsrs_deck *d, uint64_t now, fsrs_stats *out);

/* ───────────────────────────────────────────────────────────────
 * Persistence  (simple binary format)
 * ─────────────────────────────────────────────────────────────── */
bool fsrs_save(const fsrs_deck *d, const char *path);
bool fsrs_load(fsrs_deck *d, const char *path, const fsrs_params *params);

/* ───────────────────────────────────────────────────────────────
 * Utilities
 * ─────────────────────────────────────────────────────────────── */

/* Compute retrievability (0..1) given stability and elapsed seconds. */
static inline float fsrs_retrievability(float stability, uint64_t elapsed_secs) {
    if (stability <= 0.0f) return 0.0f;
    double t = (double)elapsed_secs / FSRS_SEC_PER_DAY;
    /* R(t,S) = (1 + t/(9·S))^(−1)  — FSRS-5 formula */
    double r = 1.0 / (1.0 + t / (9.0 * (double)stability));
    return (float)(r < 0.0 ? 0.0 : r > 1.0 ? 1.0 : r);
}

/* Format an interval (seconds) into a human-readable string. */
void fsrs_format_interval(uint64_t secs, char *buf, size_t len);

/* Seed the internal PRNG (used for shuffling new cards). */
void fsrs_seed(uint64_t seed);

/* Logical day index for a given timestamp. */
uint64_t fsrs_day_of(const fsrs_deck *d, uint64_t ts);

/* Unix timestamp of the start of a logical day. */
uint64_t fsrs_day_start(const fsrs_deck *d, uint64_t day);

#ifdef __cplusplus
}
#endif

#endif /* FSRS_H */