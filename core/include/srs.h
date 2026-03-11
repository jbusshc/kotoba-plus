#ifndef SRS_H
#define SRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────
 *  Time model
 * ───────────────────────────────────────────────────────────── */

#define SRS_DAY 86400ULL

/* Día comienza a las 04:00 AM */
#define SRS_DAY_START_OFFSET (4 * 3600ULL)

static inline uint64_t srs_today(uint64_t now)
{
    if (now < SRS_DAY_START_OFFSET)
        return 0;
    return (now - SRS_DAY_START_OFFSET) / SRS_DAY;
}

static inline uint64_t srs_day_to_unix(uint64_t day)
{
    return day * SRS_DAY + SRS_DAY_START_OFFSET;
}

/* ─────────────────────────────────────────────────────────────
 *  Spaced Repetition Algorithm, adapted from SM-2 and FSRS
 * ───────────────────────────────────────────────────────────── */

typedef enum {
    SRS_MODE_SM2 = 0,
    SRS_MODE_FSRS
} srs_mode;

/* ─────────────────────────────────────────────────────────────
 *  Review quality
 * ───────────────────────────────────────────────────────────── */

typedef enum {
    SRS_AGAIN = 0,
    SRS_HARD  = 3,
    SRS_GOOD  = 4,
    SRS_EASY  = 5
} srs_quality;

/* ─────────────────────────────────────────────────────────────
 *  Card state
 * ───────────────────────────────────────────────────────────── */

typedef enum {
    SRS_STATE_NEW = 0,
    SRS_STATE_LEARNING,
    SRS_STATE_RELEARNING,
    SRS_STATE_REVIEW,
    SRS_STATE_SUSPENDED
} srs_state;

/* ─────────────────────────────────────────────────────────────
 *  Flags
 * ───────────────────────────────────────────────────────────── */

#define SRS_FLAG_LEECH   0x01
#define SRS_FLAG_BURIED  0x02

/* ─────────────────────────────────────────────────────────────
 *  Persistent card
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    uint32_t entry_id;
    uint8_t state;
    uint8_t step;
    uint8_t flags;
    uint8_t reserved;

    uint64_t due;      /* unix seconds */

    uint16_t interval;
    uint16_t reps;
    uint16_t lapses;

    float ease;
    float retention;    /* retención percibida (0..1) */
    float stability;    /* cuánto tiempo el item "sobrevive" */
    float difficulty;   /* dificultad percibida (0.3–1.8 típicamente) */
} srs_item;

/* ─────────────────────────────────────────────────────────────
 *  In-memory profile with queues
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    srs_item *items;
    uint32_t  count;
    uint32_t  capacity;

    /* review heap (min-heap by due) - stores indices into items[] */
    uint32_t *review_heap;
    uint32_t  review_heap_size;

    /* learning heap (min-heap by due) */
    uint32_t *learning_heap;
    uint32_t  learning_heap_size;

    /* interday learning heap (due by day) */
    uint32_t *day_learning_heap;
    uint32_t  day_learning_heap_size;


    /* new queue (randomized stack) - stores indices into items[] */
    uint32_t *new_stack;
    uint32_t  new_stack_size;

    /* bitmap of membership (same meaning as before) */
    uint8_t  *bitmap;
    uint32_t  dict_size;

    /* daily limits / counters (in-memory, not persisted to keep format) */
    uint32_t daily_new_limit;
    uint32_t daily_review_limit;

    uint64_t current_logical_day;
    uint32_t new_served_today;
    uint32_t review_served_today;

    /* learning steps (configurable) */
    uint64_t *learning_steps;
    uint32_t  learning_steps_count;

    srs_mode mode;
} srs_profile;

/* ───────────────────────────────────────────────────────────── */

typedef struct {
    srs_item *item;
    uint32_t index;
} srs_review;

/* ───────────────────────────────────────────────────────────── */

typedef struct {
    uint32_t total_cards;
    uint32_t new_count;
    uint32_t learning_count;
    uint32_t relearning_count;
    uint32_t review_count;
    uint32_t mature_count;
    uint32_t suspended_count;
    uint32_t leech_count;
    uint32_t due_now;
} srs_stats;

static inline void srs_set_mode(srs_profile *p, srs_mode mode)
{
    if (!p) return;
    p->mode = mode;
}


/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/* lifecycle */
bool srs_init(srs_profile *p, uint32_t dict_size);
void srs_free(srs_profile *p);

/* persistence (keeps old on-disk layout: dict_size, count, items, bitmap) */
bool srs_load(srs_profile *p, const char *path, uint32_t dict_size);
bool srs_save(const srs_profile *p, const char *path);

/* membership */
static inline bool srs_contains(const srs_profile *p, uint32_t entry_id)
{
    if (entry_id >= p->dict_size)
        return false;

    return (p->bitmap[entry_id >> 3] >> (entry_id & 7)) & 1;
}

/* add/remove */
bool srs_add(srs_profile *p, uint32_t entry_id, uint64_t now);
bool srs_remove(srs_profile *p, uint32_t entry_id);

/* queue management */
void srs_heapify(srs_profile *p);

/* peek/pop next due according to policy:
   priority: learning_due -> review_due -> new (if limits allow) */
srs_item *srs_peek_due(srs_profile *p, uint64_t now);
bool srs_pop_due_review(srs_profile *p, srs_review *out);

/* requeue after update (push item index into appropriate queue) */
void srs_requeue(srs_profile *p, uint32_t index);

/* answer a card (updates fields, but doesn't requeue automatically) */
void srs_answer(srs_profile *p, srs_item *it, srs_quality q, uint64_t now);
// Implementaciones internas separadas
void srs_answer_sm2(srs_profile *p, srs_item *it, srs_quality q, uint64_t now);
void srs_answer_fsrs(srs_profile *p, srs_item *it, srs_quality q, uint64_t now);
/* stats */
void srs_compute_stats(const srs_profile *p,
                       uint64_t now,
                       srs_stats *out);

/* helpers */
static inline uint64_t srs_now(void)
{
    time_t t = time(NULL);
    return (t < 0) ? 0 : (uint64_t)t;
}

static inline bool srs_is_mature(const srs_item *it)
{
    return it->state == SRS_STATE_REVIEW && it->interval >= 21;
}

/* ---------------------------
   Configuration helpers
   --------------------------- */

/* set daily limits (0 = unlimited) */
void srs_set_limits(srs_profile *p,
                    uint32_t daily_new_limit,
                    uint32_t daily_review_limit);

/* set learning steps (array copy) */
bool srs_set_learning_steps(srs_profile *p,
                            const uint64_t *steps,
                            uint32_t count);

/* random seed control (optional) */
void srs_seed_random(uint32_t seed);

bool srs_pop_due_review(srs_profile *p, srs_review *out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif