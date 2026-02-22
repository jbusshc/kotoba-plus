#include "srs.h"
#include <stdlib.h>
#include <string.h>

/* ───────────────────────────────────────────── */

static const uint64_t learning_steps[] = {
    10 * 60,
    50 * 60,
    SRS_DAY
};

#define LEARNING_STEPS_COUNT \
    (sizeof(learning_steps)/sizeof(learning_steps[0]))

#define LEECH_THRESHOLD 8

/* ───────────────────────────────────────────── */
/* Bitmap helpers */

static inline void bitmap_set(uint8_t *bm, uint32_t id)
{
    bm[id >> 3] |= (1u << (id & 7));
}

static inline void bitmap_clear(uint8_t *bm, uint32_t id)
{
    bm[id >> 3] &= ~(1u << (id & 7));
}

/* ───────────────────────────────────────────── */
/* Heap */

static inline bool heap_less(const srs_profile *p,
                             uint32_t a,
                             uint32_t b)
{
    return p->items[a].due < p->items[b].due;
}

static void heap_swap(uint32_t *a, uint32_t *b)
{
    uint32_t t = *a; *a = *b; *b = t;
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

/* ───────────────────────────────────────────── */
/* Lifecycle */

bool srs_init(srs_profile *p, uint32_t dict_size)
{
    memset(p, 0, sizeof(*p));

    p->dict_size = dict_size;
    p->bitmap = calloc((dict_size + 7)/8, 1);

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

/* ───────────────────────────────────────────── */
/* Scheduling */

srs_item *srs_peek_due(srs_profile *p, uint64_t now)
{
    if (!p->heap_size) return NULL;
    srs_item *it = &p->items[p->heap[0]];
    return (it->due <= now) ? it : NULL;
}

bool srs_pop_due_review(srs_profile *p, srs_review *out)
{
    if (!p->heap_size) return false;
    uint32_t idx = heap_pop(p);
    out->index = idx;
    out->item  = &p->items[idx];
    return true;
}

void srs_requeue(srs_profile *p, uint32_t index)
{
    heap_push(p, index);
}

/* ───────────────────────────────────────────── */
/* Core Logic */

void srs_answer(srs_item *it, srs_quality q, uint64_t now)
{
    if (it->state == SRS_STATE_SUSPENDED)
        return;

    /* NEW → LEARNING */
    if (it->state == SRS_STATE_NEW) {
        it->state = SRS_STATE_LEARNING;
        it->step = 0;
    }

    /* LEARNING / RELEARNING */
    if (it->state == SRS_STATE_LEARNING ||
        it->state == SRS_STATE_RELEARNING)
    {
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

        /* Graduate to REVIEW (día lógico) */
        it->state = SRS_STATE_REVIEW;
        it->reps = 1;
        it->interval = 1;

        uint64_t today = srs_today(now);
        uint64_t due_day = today + 1;
        it->due = srs_day_to_unix(due_day);

        return;
    }

    /* REVIEW */
    if (q < 3) {
        it->lapses++;

        if (it->lapses >= LEECH_THRESHOLD)
            it->flags |= SRS_FLAG_LEECH;

        it->state = SRS_STATE_RELEARNING;
        it->step = 0;
        it->interval = 0;

        if (it->ease > 1.3f)
            it->ease -= 0.15f;

        it->due = now + learning_steps[0];
        return;
    }

    /* REVIEW success */

    if (it->reps == 1)
        it->interval = 6;
    else
        it->interval = (uint16_t)(it->interval * it->ease);

    it->reps++;

    uint64_t today = srs_today(now);
    uint64_t due_day = today + it->interval;

    it->due = srs_day_to_unix(due_day);
}


/* ───────────────────────────────────────────── */

bool srs_add(srs_profile *p, uint32_t entry_id, uint64_t now)
{
    if (entry_id >= p->dict_size) return false;
    if (srs_contains(p, entry_id)) return false;

    if (p->count == p->capacity) {
        p->capacity *= 2;
        p->items = realloc(p->items,
                           p->capacity*sizeof(srs_item));
        p->heap = realloc(p->heap,
                          p->capacity*sizeof(uint32_t));
    }

    uint32_t idx = p->count++;
    srs_item *it = &p->items[idx];

    it->entry_id = entry_id;
    it->due = now;
    it->interval = 0;
    it->reps = 0;
    it->lapses = 0;
    it->ease = 2.5f;
    it->state = SRS_STATE_NEW;
    it->step = 0;
    it->flags = 0;

    bitmap_set(p->bitmap, entry_id);
    heap_push(p, idx);
    return true;
}

bool srs_remove(srs_profile *p, uint32_t entry_id)
{
    if (entry_id >= p->dict_size) return false;
    if (!srs_contains(p, entry_id)) return false;

    for (uint32_t i = 0; i < p->count; ++i) {
        if (p->items[i].entry_id == entry_id) {
            bitmap_clear(p->bitmap, entry_id);
            p->items[i] = p->items[--p->count];
            // 🔥 CRÍTICO: reconstruir heap
            srs_heapify(p);
            return true;
        }
    }

    return false;
}

bool srs_save(const srs_profile *p,
              const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
        return false;

    // dict_size
    if (fwrite(&p->dict_size, sizeof(p->dict_size), 1, fp) != 1)
        goto fail;

    // count
    if (fwrite(&p->count, sizeof(p->count), 1, fp) != 1)
        goto fail;

    // items
    if (p->count > 0)
    {
        if (fwrite(p->items,
                   sizeof(srs_item),
                   p->count,
                   fp) != p->count)
            goto fail;
    }

    // bitmap
    uint32_t bitmap_size = (p->dict_size + 7) / 8;
    if (bitmap_size > 0)
    {
        if (fwrite(p->bitmap,
                   1,
                   bitmap_size,
                   fp) != bitmap_size)
            goto fail;
    }

    fclose(fp);
    return true;

fail:
    fclose(fp);
    return false;
}

bool srs_load(srs_profile *p,
              const char *path,
              uint32_t expected_dict_size)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return false;

    uint32_t dict_size = 0;
    uint32_t count = 0;

    if (fread(&dict_size, sizeof(dict_size), 1, fp) != 1)
        goto fail;

    if (dict_size != expected_dict_size)
        goto fail;

    if (fread(&count, sizeof(count), 1, fp) != 1)
        goto fail;

    // limpiar perfil actual
    srs_free(p);
    srs_init(p, dict_size);

    // reservar items
    if (count > 0)
    {
        if (count > p->capacity)
        {
            p->items = realloc(p->items,
                               sizeof(srs_item) * count);
            p->capacity = count;
        }

        if (fread(p->items,
                  sizeof(srs_item),
                  count,
                  fp) != count)
            goto fail;

        p->count = count;
    }

    // bitmap
    uint32_t bitmap_size = (dict_size + 7) / 8;
    if (bitmap_size > 0)
    {
        if (fread(p->bitmap,
                  1,
                  bitmap_size,
                  fp) != bitmap_size)
            goto fail;
    }

    fclose(fp);

    // 🔥 CRÍTICO: reconstruir heap
    srs_heapify(p);

    return true;

fail:
    fclose(fp);
    return false;
}


void srs_heapify(srs_profile *p)
{
    /* --------------------------------------------------
       1) limpiar bitmap completamente
    -------------------------------------------------- */
    uint32_t bitmap_size = (p->dict_size + 7) / 8;
    memset(p->bitmap, 0, bitmap_size);

    /* --------------------------------------------------
       2) reconstruir bitmap
    -------------------------------------------------- */
    for (uint32_t i = 0; i < p->count; ++i)
    {
        uint32_t id = p->items[i].entry_id;
        p->bitmap[id >> 3] |= (1u << (id & 7));
    }

    /* --------------------------------------------------
       3) reconstruir heap array (índices)
    -------------------------------------------------- */
    for (uint32_t i = 0; i < p->count; ++i)
        p->heap[i] = i;

    p->heap_size = p->count;

    /* --------------------------------------------------
       4) heapify bottom-up (min-heap por due)
    -------------------------------------------------- */
    for (int32_t i = (int32_t)p->heap_size / 2 - 1;
         i >= 0;
         --i)
    {
        uint32_t idx = (uint32_t)i;

        while (1)
        {
            uint32_t l = (idx << 1) + 1;
            uint32_t r = l + 1;
            uint32_t smallest = idx;

            if (l < p->heap_size &&
                p->items[p->heap[l]].due <
                p->items[p->heap[smallest]].due)
                smallest = l;

            if (r < p->heap_size &&
                p->items[p->heap[r]].due <
                p->items[p->heap[smallest]].due)
                smallest = r;

            if (smallest == idx)
                break;

            uint32_t tmp = p->heap[idx];
            p->heap[idx] = p->heap[smallest];
            p->heap[smallest] = tmp;

            idx = smallest;
        }
    }
}

/* ───────────────────────────────────────────── */
/* Stats */

void srs_compute_stats(const srs_profile *p,
                       uint64_t now,
                       srs_stats *out)
{
    memset(out, 0, sizeof(*out));

    out->total_cards = p->count;

    for (uint32_t i = 0; i < p->count; ++i) {
        const srs_item *it = &p->items[i];

        if (it->due <= now)
            out->due_now++;

        switch (it->state) {
        case SRS_STATE_NEW:
            out->new_count++;
            break;
        case SRS_STATE_LEARNING:
            out->learning_count++;
            break;
        case SRS_STATE_RELEARNING:
            out->relearning_count++;
            break;
        case SRS_STATE_REVIEW:
            out->review_count++;
            if (srs_is_mature(it))
                out->mature_count++;
            break;
        case SRS_STATE_SUSPENDED:
            out->suspended_count++;
            break;
        }

        if (it->flags & SRS_FLAG_LEECH)
            out->leech_count++;
    }
}
