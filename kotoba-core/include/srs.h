#ifndef SRS_H
#define SRS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "kotoba.h"

/* ─────────────────────────────────────────────────────────────
 *  Time model
 * ───────────────────────────────────────────────────────────── */

#define SRS_DAY 86400ULL

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
 *  Card state (robust model)
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

    uint64_t due;

    uint16_t interval;
    uint16_t reps;
    uint16_t lapses;

    float ease;

    uint8_t state;
    uint8_t step;
    uint8_t flags;
    uint8_t reserved;
} srs_item;

/* ───────────────────────────────────────────────────────────── */

typedef struct {
    srs_item *item;
    uint32_t index;
} srs_review;

/* ───────────────────────────────────────────────────────────── */

typedef struct {
    srs_item *items;
    uint32_t  count;
    uint32_t  capacity;

    uint32_t *heap;
    uint32_t  heap_size;

    uint8_t  *bitmap;
    uint32_t  dict_size;
} srs_profile;

/* ─────────────────────────────────────────────────────────────
 *  Dashboard statistics
 * ───────────────────────────────────────────────────────────── */

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

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

KOTOBA_API bool srs_init(srs_profile *p, uint32_t dict_size);
KOTOBA_API void srs_free(srs_profile *p);

KOTOBA_API bool srs_load(srs_profile *p, const char *path, uint32_t dict_size);
KOTOBA_API bool srs_save(const srs_profile *p, const char *path);

static inline bool srs_contains(const srs_profile *p, uint32_t entry_id)
{
    return (p->bitmap[entry_id >> 3] >> (entry_id & 7)) & 1;
}

KOTOBA_API bool srs_add(srs_profile *p, uint32_t entry_id, uint64_t now);
KOTOBA_API bool srs_remove(srs_profile *p, uint32_t entry_id);
KOTOBA_API void srs_heapify(srs_profile *p);

KOTOBA_API srs_item *srs_peek_due(srs_profile *p, uint64_t now);
KOTOBA_API bool srs_pop_due_review(srs_profile *p, srs_review *out);
KOTOBA_API void srs_requeue(srs_profile *p, uint32_t index);

KOTOBA_API void srs_answer(srs_item *item, srs_quality q, uint64_t now);

KOTOBA_API void srs_compute_stats(const srs_profile *p,
                                  uint64_t now,
                                  srs_stats *out);

/* ───────────────────────────────────────────────────────────── */

static inline uint64_t srs_now(void)
{
    time_t t = time(NULL);
    return (t < 0) ? 0 : (uint64_t)t;
}

static inline bool srs_is_mature(const srs_item *it)
{
    return it->state == SRS_STATE_REVIEW && it->interval >= 21;
}

#endif
