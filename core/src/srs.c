#include "srs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

/* ─────────────────────────────────────────────
   Defaults
   ───────────────────────────────────────────── */

/* Default learning steps (seconds) */
static const uint64_t default_learning_steps[] = {
    10 * 60,   /* 10 min */
    50 * 60,   /* 50 min */
    SRS_DAY    /* 1 day */
};

#define DEFAULT_LEARNING_STEPS_COUNT (sizeof(default_learning_steps) / sizeof(default_learning_steps[0]))

/* fuzz factor ±5% for intervals in days */
#define INTERVAL_FUZZ_PCT 0.05f

/* leech threshold */
#define LEECH_THRESHOLD 8

/* random seed guard */
static int srs_random_seeded = 0;

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
/* Random helpers */

void srs_seed_random(uint32_t seed)
{
    srand(seed);
    srs_random_seeded = 1;
}

static inline uint32_t srs_rand_u32(void)
{
    if (!srs_random_seeded) {
        srand((unsigned)time(NULL));
        srs_random_seeded = 1;
    }
    return ((uint32_t)rand() << 16) ^ (uint32_t)rand();
}

static inline double srs_random_double(double low, double high)
{
    uint32_t r = srs_rand_u32();
    double unit = (double)r / (double)UINT32_MAX;
    return low + unit * (high - low);
}

/* ───────────────────────────────────────────── */
/* Generic heap ops (min-heap by item->due). heap stores indices into p->items[].
   We implement generic functions that operate on a heap array with its size. */

static inline bool heap_less_items(const srs_profile *p, uint32_t ia, uint32_t ib)
{
    return p->items[ia].due < p->items[ib].due;
}

static void heap_sift_up(srs_profile *p,
                         uint32_t *heap,
                         uint32_t heap_size,
                         uint32_t idx_pos)
{
    while (idx_pos > 0) {
        uint32_t parent = (idx_pos - 1) >> 1;
        if (!heap_less_items(p, heap[idx_pos], heap[parent]))
            break;
        uint32_t tmp = heap[idx_pos];
        heap[idx_pos] = heap[parent];
        heap[parent] = tmp;
        idx_pos = parent;
    }
}

static void heap_sift_down(srs_profile *p,
                           uint32_t *heap,
                           uint32_t heap_size,
                           uint32_t idx_pos)
{
    while (1) {
        uint32_t l = (idx_pos << 1) + 1;
        uint32_t r = l + 1;
        uint32_t smallest = idx_pos;

        if (l < heap_size && heap_less_items(p, heap[l], heap[smallest])) smallest = l;
        if (r < heap_size && heap_less_items(p, heap[r], heap[smallest])) smallest = r;
        if (smallest == idx_pos) break;

        uint32_t tmp = heap[idx_pos];
        heap[idx_pos] = heap[smallest];
        heap[smallest] = tmp;
        idx_pos = smallest;
    }
}

static void heap_push(srs_profile *p, uint32_t *heap, uint32_t *heap_size, uint32_t item_idx)
{
    heap[*heap_size] = item_idx;
    (*heap_size)++;
    heap_sift_up(p, heap, *heap_size, *heap_size - 1);
}

/* wrappers específicos para cada cola */

static inline void heap_push_learning(srs_profile *p, uint32_t idx)
{
    heap_push(p, p->learning_heap, &p->learning_heap_size, idx);
}

static inline void heap_push_review(srs_profile *p, uint32_t idx)
{
    heap_push(p, p->review_heap, &p->review_heap_size, idx);
}

static inline void heap_push_day_learning(srs_profile *p, uint32_t idx)
{
    heap_push(p, p->day_learning_heap, &p->day_learning_heap_size, idx);
}


static uint32_t heap_pop(srs_profile *p, uint32_t *heap, uint32_t *heap_size)
{
    uint32_t top = heap[0];

    (*heap_size)--;

    if (*heap_size > 0) {
        heap[0] = heap[*heap_size];
        heap_sift_down(p, heap, *heap_size, 0);
    }

    return top;
}

/* ───────────────────────────────────────────── */
/* Utility: shuffle new_stack (Fisher-Yates) */

static void shuffle_new_stack(srs_profile *p)
{
    if (!p->new_stack || p->new_stack_size <= 1) return;
    for (uint32_t i = p->new_stack_size - 1; i > 0; --i) {
        uint32_t j = srs_rand_u32() % (i + 1);
        uint32_t tmp = p->new_stack[i];
        p->new_stack[i] = p->new_stack[j];
        p->new_stack[j] = tmp;
    }
}

static void srs_promote_day_learning(srs_profile *p, uint64_t now)
{
    uint64_t today = srs_today(now);

    while (p->day_learning_heap_size > 0)
    {
        uint32_t idx = p->day_learning_heap[0];
        srs_item *it = &p->items[idx];

        if (srs_today(it->due) > today)
            break;

        heap_pop(p, p->day_learning_heap, &p->day_learning_heap_size);
        heap_push_learning(p, idx);
    }
}

/* ───────────────────────────────────────────── */
/* Lifecycle */

bool srs_init(srs_profile *p, uint32_t dict_size)
{
    if (!p) return false;
    memset(p, 0, sizeof(*p));

    p->dict_size = dict_size;
    uint32_t bitmap_size = (dict_size + 7) / 8;
    p->bitmap = calloc(bitmap_size, 1);
    if (!p->bitmap) return false;

    p->capacity = 1024;
    p->items = malloc(p->capacity * sizeof(srs_item));
    p->review_heap = malloc(p->capacity * sizeof(uint32_t));
    p->learning_heap = malloc(p->capacity * sizeof(uint32_t));
    p->new_stack = malloc(p->capacity * sizeof(uint32_t));
    p->day_learning_heap = malloc(sizeof(uint32_t) * p->capacity);
    if (!p->items || !p->review_heap || !p->learning_heap || !p->new_stack || !p->day_learning_heap) {
        srs_free(p);
        return false;
    }

    p->count = 0;
    p->review_heap_size = 0;
    p->learning_heap_size = 0;
    p->new_stack_size = 0;
    p->day_learning_heap_size = 0;

    /* defaults */
    p->daily_new_limit = 20;
    p->daily_review_limit = 0; /* 0 = unlimited */

    /* copy default learning steps */
    p->learning_steps_count = DEFAULT_LEARNING_STEPS_COUNT;
    p->learning_steps = malloc(sizeof(uint64_t) * p->learning_steps_count);
    if (!p->learning_steps) {
        srs_free(p);
        return false;
    }
    for (uint32_t i = 0; i < p->learning_steps_count; ++i)
        p->learning_steps[i] = default_learning_steps[i];

    p->current_logical_day = srs_today(srs_now());
    p->new_served_today = 0;
    p->review_served_today = 0;

    return true;
}

void srs_free(srs_profile *p)
{
    if (!p) return;
    free(p->items);
    free(p->review_heap);
    free(p->learning_heap);
    free(p->new_stack);
    free(p->bitmap);
    free(p->learning_steps);
    free(p->day_learning_heap);
    memset(p, 0, sizeof(*p));
}

/* ───────────────────────────────────────────── */
/* Scheduling helpers */

/* Check & reset daily counters on day change */
static void srs_update_day_if_needed(srs_profile *p, uint64_t now)
{
    uint64_t today = srs_today(now);
    if (p->current_logical_day != today) {
        p->current_logical_day = today;
        p->new_served_today = 0;
        p->review_served_today = 0;
    }
}
/* Rebuild queues (from items[]). Called after load or structural changes. */
void srs_heapify(srs_profile *p)
{
    if (!p) return;

    /* clear bitmap and rebuild */
    uint32_t bitmap_size = (p->dict_size + 7) / 8;
    if (p->bitmap && bitmap_size > 0)
        memset(p->bitmap, 0, bitmap_size);

    /* ensure capacity for arrays */
    if (p->capacity < p->count) {
        p->items = realloc(p->items, sizeof(srs_item) * p->count);
        p->review_heap = realloc(p->review_heap, sizeof(uint32_t) * p->count);
        p->learning_heap = realloc(p->learning_heap, sizeof(uint32_t) * p->count);
        p->new_stack = realloc(p->new_stack, sizeof(uint32_t) * p->count);
        p->day_learning_heap = realloc(p->day_learning_heap, sizeof(uint32_t) * p->count);

        p->capacity = p->count;
    }

    p->learning_heap_size = 0;
    p->day_learning_heap_size = 0;
    p->review_heap_size = 0;
    p->new_stack_size = 0;

    uint64_t now = srs_now();
    uint64_t today = srs_today(now);

    for (uint32_t i = 0; i < p->count; ++i) {
        uint32_t id = p->items[i].entry_id;
        if (id < p->dict_size)
            bitmap_set(p->bitmap, id);

        srs_item *it = &p->items[i];

        if (it->state == SRS_STATE_NEW) {
            p->new_stack[p->new_stack_size++] = i;
            continue;
        }

        if (it->state == SRS_STATE_LEARNING || it->state == SRS_STATE_RELEARNING) {
            uint64_t due_day = srs_today(it->due);
            if (due_day == today) {
                heap_push_learning(p, i);
            } else {
                heap_push_day_learning(p, i);
            }
            continue;
        }

        if (it->state == SRS_STATE_REVIEW) {
            heap_push_review(p, i);
        }
    }

    /* shuffle new_stack for randomized new order */
    shuffle_new_stack(p);
}

/* ───────────────────────────────────────────── */
/* Core Logic: peek / pop according to policy:
   Priority: learning_due -> review_due -> new (if daily limit allows)
*/

srs_item *srs_peek_due(srs_profile *p, uint64_t now)
{
    if (!p) return NULL;

    srs_update_day_if_needed(p, now);
    srs_promote_day_learning(p, now);

    /* learning due first */
    if (p->learning_heap_size > 0) {
        uint32_t idx = p->learning_heap[0];
        if (p->items[idx].due <= now)
            return &p->items[idx];
    }

    /* then review due */
    if (p->review_heap_size > 0) {
        uint32_t idx = p->review_heap[0];
        if (p->items[idx].due <= now) {
            /* check review daily limit */
            if (p->daily_review_limit == 0 || p->review_served_today < p->daily_review_limit)
                return &p->items[idx];
        }
    }

    /* then new (if limit not reached) */
    if (p->new_stack_size > 0) {
        if (p->daily_new_limit == 0 || p->new_served_today < p->daily_new_limit) {
            /* new_stack used as stack: top is last element */
            uint32_t idx = p->new_stack[p->new_stack_size - 1];
            return &p->items[idx];
        }
    }

    return NULL;
}

bool srs_pop_due_review(srs_profile *p, srs_review *out)
{
    if (!p || !out) return false;

    uint64_t now = srs_now();
    srs_update_day_if_needed(p, now);
    srs_promote_day_learning(p, now);
    /* learning due first */
    if (p->learning_heap_size > 0) {
        uint32_t idx = p->learning_heap[0];
        if (p->items[idx].due <= now) {
            uint32_t item_index = heap_pop(p, p->learning_heap, &p->learning_heap_size);
            out->index = item_index;
            out->item = &p->items[item_index];
            /* learning counts as review for daily limits? Often treated separate.
               We'll treat learning as 'review' for daily_review_limit bookkeeping. */
            if (p->daily_review_limit != 0)
                p->review_served_today++;
            return true;
        }
    }

    /* review due */
    if (p->review_heap_size > 0) {
        uint32_t idx = p->review_heap[0];
        if (p->items[idx].due <= now) {
            if (p->daily_review_limit != 0 && p->review_served_today >= p->daily_review_limit)
                return false; /* limit reached */
            uint32_t item_index = heap_pop(p, p->review_heap, &p->review_heap_size);
            out->index = item_index;
            out->item = &p->items[item_index];
            if (p->daily_review_limit != 0)
                p->review_served_today++;
            return true;
        }
    }

    /* new */
    if (p->new_stack_size > 0) {
        if (p->daily_new_limit != 0 && p->new_served_today >= p->daily_new_limit)
            return false;

        /* pop last */
        uint32_t item_index = p->new_stack[p->new_stack_size - 1];
        p->new_stack_size--;
        out->index = item_index;
        out->item = &p->items[item_index];
        if (p->daily_new_limit != 0)
            p->new_served_today++;
        return true;
    }

    return false;
}

/* requeue: insert item index back to appropriate queue according to state/due */
void srs_requeue(srs_profile *p, uint32_t index)
{
    if (!p) return;
    if (index >= p->count) return;

    srs_item *it = &p->items[index];

    if (it->state == SRS_STATE_NEW) {

        if (p->new_stack_size == p->capacity) {
            p->capacity *= 2;
            p->new_stack = realloc(p->new_stack, sizeof(uint32_t) * p->capacity);
        }

        p->new_stack[p->new_stack_size++] = index;

        if (p->new_stack_size > 1) {
            uint32_t j = srs_rand_u32() % p->new_stack_size;
            uint32_t tmp = p->new_stack[p->new_stack_size - 1];
            p->new_stack[p->new_stack_size - 1] = p->new_stack[j];
            p->new_stack[j] = tmp;
        }
    }

else if (it->state == SRS_STATE_LEARNING || it->state == SRS_STATE_RELEARNING) {
    heap_push_learning(p, index);
}

else if (it->state == SRS_STATE_REVIEW) {
    heap_push_review(p, index);
}
}

/* ───────────────────────────────────────────── */
/* Core SRS: answer/stat transitions */

/* Helper: apply fuzz to an interval (days) and return integer days */
static uint32_t apply_interval_fuzz(uint32_t interval_days)
{
    if (interval_days == 0) return 0;
    double low = 1.0 - INTERVAL_FUZZ_PCT;
    double high = 1.0 + INTERVAL_FUZZ_PCT;
    double factor = srs_random_double(low, high);
    double v = (double)interval_days * factor;
    uint32_t out = (uint32_t)(v + 0.5);
    if (out == 0) out = 1;
    return out;
}
void srs_answer(srs_profile *p, srs_item *it, srs_quality q, uint64_t now){
    if (!it) return;
    if (it->state == SRS_STATE_SUSPENDED)
        return;

    /* NEW -> LEARNING */
    if (it->state == SRS_STATE_NEW) {
        it->state = SRS_STATE_LEARNING;
        it->step = 0;
        it->due = now + p->learning_steps[0];
        return;
    }

    /* LEARNING / RELEARNING */
    if (it->state == SRS_STATE_LEARNING || it->state == SRS_STATE_RELEARNING)
    {
        if (q < SRS_HARD) {
            it->step = 0;
            it->due = now + p->learning_steps[0];
            return;
        }

        it->step++;

        if (it->step < p->learning_steps_count) {
            it->due = now + p->learning_steps[it->step];
            return;
        }

        /* graduate */

        it->state = SRS_STATE_REVIEW;
        it->reps = 1;

        if (q == SRS_EASY)
            it->interval = 4;
        else
            it->interval = 1;

        uint64_t today = srs_today(now);
        uint64_t due_day = today + it->interval;

        it->due = srs_day_to_unix(due_day);
        return;
    }

    /* REVIEW */
    if (it->state == SRS_STATE_REVIEW)
    {
        if (q < SRS_HARD) {

            it->lapses++;

            if (it->lapses >= LEECH_THRESHOLD)
                it->flags |= SRS_FLAG_LEECH;

            it->state = SRS_STATE_RELEARNING;
            it->step = 0;
            it->interval = 0;

            if (it->ease > 1.3f)
                it->ease -= 0.2f;

            it->due = now + p->learning_steps[0];
            return;
        }

        if (it->reps == 1) {
            it->interval = 6;
        }
        else {

            float factor;

            if (q == SRS_HARD) {
                factor = it->ease * 0.8f;
                it->ease -= 0.15f;
            }
            else if (q == SRS_GOOD) {
                factor = it->ease;
            }
            else {
                factor = it->ease * 1.3f;
                it->ease += 0.15f;
            }

            uint32_t next = (uint32_t)(it->interval * factor);

            if (next < 1)
                next = 1;

            it->interval = next;
        }

        if (it->ease < 1.3f) it->ease = 1.3f;
        if (it->ease > 2.5f) it->ease = 2.5f;

        it->reps++;

        uint32_t fuzzed = apply_interval_fuzz(it->interval);

        uint64_t today = srs_today(now);
        uint64_t due_day = today + fuzzed;

        it->due = srs_day_to_unix(due_day);
    }
}

/* ───────────────────────────────────────────── */
/* add/remove */

bool srs_add(srs_profile *p, uint32_t entry_id, uint64_t now)
{
    if (!p) return false;
    if (entry_id >= p->dict_size) return false;
    if (srs_contains(p, entry_id)) return false;

    if (p->count == p->capacity) {
        p->capacity *= 2;
        p->items = realloc(p->items, p->capacity * sizeof(srs_item));
        p->review_heap = realloc(p->review_heap, p->capacity * sizeof(uint32_t));
        p->learning_heap = realloc(p->learning_heap, p->capacity * sizeof(uint32_t));
        p->new_stack = realloc(p->new_stack, p->capacity * sizeof(uint32_t));
        p->day_learning_heap = realloc(p->day_learning_heap, p->capacity * sizeof(uint32_t));
    }

    uint32_t idx = p->count++;
    srs_item *it = &p->items[idx];

    it->entry_id = entry_id;
    it->due = now; /* new available immediately */

    it->interval = 0;
    it->reps = 0;
    it->lapses = 0;
    it->ease = 2.5f;
    it->state = SRS_STATE_NEW;
    it->step = 0;
    it->flags = 0;

    bitmap_set(p->bitmap, entry_id);

    /* push into new_stack (stack behavior) */
    p->new_stack[p->new_stack_size++] = idx;
    /* small randomization to avoid strictly FIFO */
    if (p->new_stack_size > 1) {
        uint32_t j = srs_rand_u32() % p->new_stack_size;
        uint32_t tmp = p->new_stack[p->new_stack_size - 1];
        p->new_stack[p->new_stack_size - 1] = p->new_stack[j];
        p->new_stack[j] = tmp;
    }

    return true;
}

bool srs_remove(srs_profile *p, uint32_t entry_id)
{
    if (!p) return false;
    if (entry_id >= p->dict_size) return false;
    if (!srs_contains(p, entry_id)) return false;

    for (uint32_t i = 0; i < p->count; ++i) {
        if (p->items[i].entry_id == entry_id) {
            bitmap_clear(p->bitmap, entry_id);
            /* remove by swapping with last item */
            p->items[i] = p->items[--p->count];
            /* rebuild queues to remain consistent */
            srs_heapify(p);
            return true;
        }
    }
    return false;
}

/* ───────────────────────────────────────────── */
/* persistence: keep same format (dict_size, count, items, bitmap) */

bool srs_save(const srs_profile *p, const char *path)
{
    if (!p || !path) return false;
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    if (fwrite(&p->dict_size, sizeof(p->dict_size), 1, fp) != 1)
        goto fail;

    if (fwrite(&p->count, sizeof(p->count), 1, fp) != 1)
        goto fail;

    if (p->count > 0) {
        if (fwrite(p->items, sizeof(srs_item), p->count, fp) != p->count)
            goto fail;
    }

    uint32_t bitmap_size = (p->dict_size + 7) / 8;
    if (bitmap_size > 0) {
        if (fwrite(p->bitmap, 1, bitmap_size, fp) != bitmap_size)
            goto fail;
    }

    fclose(fp);
    return true;

fail:
    fclose(fp);
    return false;
}

bool srs_load(srs_profile *p, const char *path, uint32_t expected_dict_size)
{
    if (!p || !path) return false;
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    uint32_t dict_size = 0;
    uint32_t count = 0;

    if (fread(&dict_size, sizeof(dict_size), 1, fp) != 1)
        goto fail;

    if (dict_size != expected_dict_size)
        goto fail;

    if (fread(&count, sizeof(count), 1, fp) != 1)
        goto fail;

    /* reset current profile */
    srs_free(p);
    if (!srs_init(p, dict_size))
        goto fail;

    /* reserve items */
    if (count > 0) {
        if (count > p->capacity) {
            p->items = realloc(p->items, sizeof(srs_item) * count);
            p->review_heap = realloc(p->review_heap, sizeof(uint32_t) * count);
            p->learning_heap = realloc(p->learning_heap, sizeof(uint32_t) * count);
            p->new_stack = realloc(p->new_stack, sizeof(uint32_t) * count);
            p->day_learning_heap = realloc(p->day_learning_heap, sizeof(uint32_t) * count);
            p->capacity = count;
        }

        if (fread(p->items, sizeof(srs_item), count, fp) != count)
            goto fail;
        p->count = count;
    }

    /* bitmap */
    uint32_t bitmap_size = (dict_size + 7) / 8;
    if (bitmap_size > 0) {
        if (fread(p->bitmap, 1, bitmap_size, fp) != bitmap_size)
            goto fail;
    }

    fclose(fp);

    /* rebuild queues from items */
    srs_heapify(p);
    return true;

fail:
    fclose(fp);
    return false;
}

/* ───────────────────────────────────────────── */
/* Stats */

void srs_compute_stats(const srs_profile *p,
                       uint64_t now,
                       srs_stats *out)
{
    if (!p || !out) return;
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

/* ───────────────────────────────────────────── */
/* Config helpers */

void srs_set_limits(srs_profile *p,
                    uint32_t daily_new_limit,
                    uint32_t daily_review_limit)
{
    if (!p) return;
    p->daily_new_limit = daily_new_limit;
    p->daily_review_limit = daily_review_limit;
}

bool srs_set_learning_steps(srs_profile *p,
                            const uint64_t *steps,
                            uint32_t count)
{
    if (!p || !steps || count == 0) return false;

    uint64_t *tmp = realloc(p->learning_steps, sizeof(uint64_t) * count);
    if (!tmp) return false;
    p->learning_steps = tmp;
    p->learning_steps_count = count;
    for (uint32_t i = 0; i < count; ++i)
        p->learning_steps[i] = steps[i];

    return true;
}

