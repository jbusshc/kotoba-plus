#ifndef SRS_H
#define SRS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "kotoba.h"

/* ─────────────────────────────────────────────────────────────
 *  Time model (unix seconds)
 * ───────────────────────────────────────────────────────────── */

#define SRS_DAY 86400ULL   /* seconds in a day */

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
 *  SRS state
 * ───────────────────────────────────────────────────────────── */

typedef enum {
    SRS_LEARNING = 0,
    SRS_REVIEW   = 1
} srs_state;

/* ─────────────────────────────────────────────────────────────
 *  SRS item (persistent)
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    uint32_t entry_id;

    uint64_t due;        /* unix timestamp (seconds) */

    uint16_t interval;  /* days (review only) */
    uint16_t reps;
    uint16_t lapses;

    float    ease;

    uint8_t  state;     /* srs_state */
    uint8_t  step;      /* learning step */
} srs_item;

/* ─────────────────────────────────────────────────────────────
 *  Review handle (ephemeral)
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    srs_item *item;
    uint32_t  index;
} srs_review;

/* ─────────────────────────────────────────────────────────────
 *  SRS profile
 * ───────────────────────────────────────────────────────────── */

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

KOTOBA_API srs_item *srs_peek_due(srs_profile *p, uint64_t now);
KOTOBA_API bool srs_pop_due_review(srs_profile *p, srs_review *out);
KOTOBA_API void srs_requeue(srs_profile *p, uint32_t index);

KOTOBA_API void srs_answer(srs_item *item, srs_quality q, uint64_t now);

/* ─────────────────────────────────────────────────────────────
 *  Time helpers (portable)
 * ───────────────────────────────────────────────────────────── */

static inline uint64_t srs_now(void)
{
    time_t t = time(NULL);
    return (t < 0) ? 0 : (uint64_t)t;
}

static inline void srs_print_time(uint64_t t)
{
    time_t tt = (time_t)t;
    struct tm *tm = localtime(&tt);
    if (!tm) return;

    printf("%04d-%02d-%02d %02d:%02d:%02d",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec);
}

#endif /* SRS_H */
