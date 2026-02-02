#include "srs.h"
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 *  Learning configuration (seconds)
 * ───────────────────────────────────────────────────────────── */

static const uint64_t learning_steps[] = {
    10 * 60,     /* 10 minutes */
    50 * 60,     /* 50 minutes */
    SRS_DAY      /* 1 day */
};

#define LEARNING_STEPS_COUNT \
    (sizeof(learning_steps) / sizeof(learning_steps[0]))

/* ─────────────────────────────────────────────────────────────
 *  Bitmap helpers
 * ───────────────────────────────────────────────────────────── */

static inline void bitmap_set(uint8_t *bm, uint32_t id)
{
    bm[id >> 3] |= (1u << (id & 7));
}

static inline void bitmap_clear(uint8_t *bm, uint32_t id)
{
    bm[id >> 3] &= ~(1u << (id & 7));
}

/* ─────────────────────────────────────────────────────────────
 *  Heap helpers (min-heap by due)
 * ───────────────────────────────────────────────────────────── */

static inline bool heap_less(const srs_profile *p, uint32_t a, uint32_t b)
{
    return p->items[a].due < p->items[b].due;
}

static void heap_swap(uint32_t *a, uint32_t *b)
{
    uint32_t t = *a;
    *a = *b;
    *b = t;
}

static void heap_sift_up(srs_profile *p, uint32_t i)
{
    while (i > 0) {
        uint32_t parent = (i - 1) >> 1;
        if (!heap_less(p, p->heap[i], p->heap[parent]))
            break;
        heap_swap(&p->heap[i], &p->heap[parent]);
        i = parent;
    }
}

static void heap_sift_down(srs_profile *p, uint32_t i)
{
    uint32_t n = p->heap_size;

    while (1) {
        uint32_t l = (i << 1) + 1;
        uint32_t r = l + 1;
        uint32_t s = i;

        if (l < n && heap_less(p, p->heap[l], p->heap[s])) s = l;
        if (r < n && heap_less(p, p->heap[r], p->heap[s])) s = r;

        if (s == i) break;
        heap_swap(&p->heap[i], &p->heap[s]);
        i = s;
    }
}

static void heap_push(srs_profile *p, uint32_t idx)
{
    p->heap[p->heap_size] = idx;
    heap_sift_up(p, p->heap_size);
    p->heap_size++;
}

static uint32_t heap_pop(srs_profile *p)
{
    uint32_t top = p->heap[0];
    p->heap_size--;
    p->heap[0] = p->heap[p->heap_size];
    heap_sift_down(p, 0);
    return top;
}

/* ─────────────────────────────────────────────────────────────
 *  Lifecycle
 * ───────────────────────────────────────────────────────────── */

bool srs_init(srs_profile *p, uint32_t dict_size)
{
    memset(p, 0, sizeof(*p));

    p->dict_size = dict_size;
    p->bitmap = calloc((dict_size + 7) / 8, 1);

    p->capacity = 1024;
    p->items = malloc(p->capacity * sizeof(srs_item));
    p->heap  = malloc(p->capacity * sizeof(uint32_t));

    return p->bitmap && p->items && p->heap;
}

void srs_free(srs_profile *p)
{
    free(p->items);
    free(p->heap);
    free(p->bitmap);
    memset(p, 0, sizeof(*p));
}

/* ─────────────────────────────────────────────────────────────
 *  Scheduling
 * ───────────────────────────────────────────────────────────── */

srs_item *srs_peek_due(srs_profile *p, uint64_t now)
{
    if (!p->heap_size) return NULL;
    srs_item *it = &p->items[p->heap[0]];
    return (it->due <= now) ? it : NULL;
}

bool srs_pop_due_review(srs_profile *p, srs_review *out)
{
    if (!p->heap_size)
        return false;

    uint32_t idx = heap_pop(p);
    out->index = idx;
    out->item  = &p->items[idx];
    return true;
}

void srs_requeue(srs_profile *p, uint32_t index)
{
    heap_push(p, index);
}

/* ─────────────────────────────────────────────────────────────
 *  SRS logic
 * ───────────────────────────────────────────────────────────── */

void srs_answer(srs_item *it, srs_quality q, uint64_t now)
{
    /* LEARNING */
    if (it->state == SRS_LEARNING) {

        if (q < 3) {
            it->step = 0;
            it->due = now + learning_steps[0];
            return;
        }

        it->step++;
        if (it->step < LEARNING_STEPS_COUNT) {
            it->due = now + learning_steps[it->step];
            return;
        }

        /* graduate */
        it->state = SRS_REVIEW;
        it->reps = 1;
        it->interval = 1;
        it->due = now + SRS_DAY;
        return;
    }

    /* REVIEW */
    if (q < 3) {
        it->lapses++;
        it->state = SRS_LEARNING;
        it->step = 0;
        it->interval = 0;

        if (it->ease > 1.3f)
            it->ease -= 0.15f;

        it->due = now + learning_steps[0];
        return;
    }

    if (it->reps == 1)
        it->interval = 6;
    else
        it->interval = (uint16_t)(it->interval * it->ease);

    it->reps++;
    it->due = now + (uint64_t)it->interval * SRS_DAY;
}

/* ─────────────────────────────────────────────────────────────
 *  Add / Persistence
 * ───────────────────────────────────────────────────────────── */

bool srs_add(srs_profile *p, uint32_t entry_id, uint64_t now)
{
    if (srs_contains(p, entry_id))
        return false;

    if (p->count == p->capacity) {
        p->capacity *= 2;
        p->items = realloc(p->items, p->capacity * sizeof(srs_item));
        p->heap  = realloc(p->heap,  p->capacity * sizeof(uint32_t));
    }

    uint32_t idx = p->count++;
    srs_item *it = &p->items[idx];

    it->entry_id = entry_id;
    it->due      = now;
    it->interval = 0;
    it->reps     = 0;
    it->lapses   = 0;
    it->ease     = 2.5f;
    it->state    = SRS_LEARNING;
    it->step     = 0;

    bitmap_set(p->bitmap, entry_id);
    heap_push(p, idx);
    return true;
}

bool srs_save(const srs_profile *p, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    fwrite(&p->count, sizeof(p->count), 1, f);
    fwrite(p->items, sizeof(srs_item), p->count, f);
    fclose(f);
    return true;
}

bool srs_load(srs_profile *p, const char *path, uint32_t dict_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    if (!srs_init(p, dict_size)) {
        fclose(f);
        return false;
    }

    fread(&p->count, sizeof(p->count), 1, f);

    if (p->count > p->capacity) {
        p->capacity = p->count;
        p->items = realloc(p->items, p->capacity * sizeof(srs_item));
        p->heap  = realloc(p->heap,  p->capacity * sizeof(uint32_t));
    }

    fread(p->items, sizeof(srs_item), p->count, f);
    fclose(f);

    for (uint32_t i = 0; i < p->count; ++i) {
        bitmap_set(p->bitmap, p->items[i].entry_id);
        p->heap[i] = i;
    }

    p->heap_size = p->count;
    for (int32_t i = (int32_t)p->heap_size / 2 - 1; i >= 0; --i)
        heap_sift_down(p, (uint32_t)i);

    return true;
}
