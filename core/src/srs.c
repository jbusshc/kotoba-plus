#include "srs.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

/* ─────────────────────────────────────────────
    Defaults & Config
   ───────────────────────────────────────────── */

static const uint64_t default_learning_steps[] = { 60, 300, 600, 86400, 172800, 345600 };
/* 1m, 5m, 10m, 1d, 2d, 4d */
#define DEFAULT_LEARNING_STEPS_COUNT \
    (sizeof(default_learning_steps) / sizeof(default_learning_steps[0]))

#define LEECH_THRESHOLD 8

static int srs_random_seeded = 0;

/* ─────────────────────────────────────────────
    Random & Bitmap helpers
   ───────────────────────────────────────────── */

static inline void bitmap_set(uint8_t *bm, uint32_t id)
{
    bm[id >> 3] |= (1u << (id & 7));
}

static inline void bitmap_clear(uint8_t *bm, uint32_t id)
{
    bm[id >> 3] &= ~(1u << (id & 7));
}

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

static inline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Small random fuzz ±3% so identical cards don't cluster */
static inline float fsrs_fuzz(void)
{
    return 0.97f + (float)(srs_rand_u32() % 7) / 100.0f;
}

/* ─────────────────────────────────────────────
    Heap ops
   ───────────────────────────────────────────── */

static inline bool heap_less_items(const srs_profile *p,
                                   uint32_t ia, uint32_t ib)
{
    return p->items[ia].due < p->items[ib].due;
}

static void heap_sift_up(srs_profile *p,
                         uint32_t *heap, uint32_t heap_size,
                         uint32_t pos)
{
    while (pos > 0) {
        uint32_t parent = (pos - 1) >> 1;
        if (!heap_less_items(p, heap[pos], heap[parent]))
            break;
        uint32_t tmp  = heap[pos];
        heap[pos]     = heap[parent];
        heap[parent]  = tmp;
        pos = parent;
    }
}

static void heap_sift_down(srs_profile *p,
                           uint32_t *heap, uint32_t heap_size,
                           uint32_t pos)
{
    while (1) {
        uint32_t l       = (pos << 1) + 1;
        uint32_t r       = l + 1;
        uint32_t smallest = pos;

        if (l < heap_size && heap_less_items(p, heap[l], heap[smallest]))
            smallest = l;
        if (r < heap_size && heap_less_items(p, heap[r], heap[smallest]))
            smallest = r;
        if (smallest == pos)
            break;

        uint32_t tmp      = heap[pos];
        heap[pos]         = heap[smallest];
        heap[smallest]    = tmp;
        pos = smallest;
    }
}

static void heap_push(srs_profile *p,
                      uint32_t *heap, uint32_t *heap_size,
                      uint32_t item_idx)
{
    heap[*heap_size] = item_idx;
    (*heap_size)++;
    heap_sift_up(p, heap, *heap_size, *heap_size - 1);
}

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

static uint32_t heap_pop(srs_profile *p,
                         uint32_t *heap, uint32_t *heap_size)
{
    uint32_t top = heap[0];
    (*heap_size)--;
    if (*heap_size > 0) {
        heap[0] = heap[*heap_size];
        heap_sift_down(p, heap, *heap_size, 0);
    }
    return top;
}

/* ─────────────────────────────────────────────
    New-stack shuffle
   ───────────────────────────────────────────── */

static void shuffle_new_stack(srs_profile *p)
{
    if (!p->new_stack || p->new_stack_size <= 1) return;
    for (uint32_t i = p->new_stack_size - 1; i > 0; --i) {
        uint32_t j   = srs_rand_u32() % (i + 1);
        uint32_t tmp = p->new_stack[i];
        p->new_stack[i] = p->new_stack[j];
        p->new_stack[j] = tmp;
    }
}

/* ─────────────────────────────────────────────
    Promote interday-learning cards that are now due today
   ───────────────────────────────────────────── */

static void srs_promote_day_learning(srs_profile *p, uint64_t now)
{
    uint64_t today = srs_today(now);
    while (p->day_learning_heap_size > 0) {
        uint32_t idx = p->day_learning_heap[0];
        if (srs_today(p->items[idx].due) > today)
            break;
        heap_pop(p, p->day_learning_heap, &p->day_learning_heap_size);
        heap_push_learning(p, idx);
    }
}

/* ─────────────────────────────────────────────
    Lifecycle: init / free
   ───────────────────────────────────────────── */

bool srs_init(srs_profile *p, uint32_t dict_size)
{
    if (!p) return false;
    memset(p, 0, sizeof(*p));

    p->dict_size = dict_size;
    uint32_t bitmap_bytes = (dict_size + 7) / 8;
    p->bitmap = calloc(bitmap_bytes, 1);
    if (!p->bitmap) return false;

    p->capacity = 1024;
    p->items            = malloc(p->capacity * sizeof(srs_item));
    p->review_heap      = malloc(p->capacity * sizeof(uint32_t));
    p->learning_heap    = malloc(p->capacity * sizeof(uint32_t));
    p->new_stack        = malloc(p->capacity * sizeof(uint32_t));
    p->day_learning_heap = malloc(p->capacity * sizeof(uint32_t));

    if (!p->items || !p->review_heap || !p->learning_heap ||
        !p->new_stack || !p->day_learning_heap) {
        srs_free(p);
        return false;
    }

    p->count                  = 0;
    p->review_heap_size       = 0;
    p->learning_heap_size     = 0;
    p->new_stack_size         = 0;
    p->day_learning_heap_size = 0;

    p->daily_new_limit    = 20;
    p->daily_review_limit = 0;

    p->learning_steps_count = DEFAULT_LEARNING_STEPS_COUNT;
    p->learning_steps = malloc(sizeof(uint64_t) * p->learning_steps_count);
    if (!p->learning_steps) {
        srs_free(p);
        return false;
    }
    for (uint32_t i = 0; i < p->learning_steps_count; ++i)
        p->learning_steps[i] = default_learning_steps[i];

    p->current_logical_day = srs_today(srs_now());
    p->new_served_today    = 0;
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

/* ─────────────────────────────────────────────
    Day rollover helper
   ───────────────────────────────────────────── */

static void srs_update_day_if_needed(srs_profile *p, uint64_t now)
{
    uint64_t today = srs_today(now);
    if (p->current_logical_day != today) {
        p->current_logical_day = today;
        p->new_served_today    = 0;
        p->review_served_today = 0;
    }
}

/* ─────────────────────────────────────────────
    Heapify (called after load)
   ───────────────────────────────────────────── */

void srs_heapify(srs_profile *p)
{
    if (!p) return;

    uint32_t bitmap_bytes = (p->dict_size + 7) / 8;
    if (p->bitmap && bitmap_bytes > 0)
        memset(p->bitmap, 0, bitmap_bytes);

    /* Ensure arrays are large enough */
    if (p->capacity < p->count) {
        p->items             = realloc(p->items,             sizeof(srs_item)   * p->count);
        p->review_heap       = realloc(p->review_heap,       sizeof(uint32_t)   * p->count);
        p->learning_heap     = realloc(p->learning_heap,     sizeof(uint32_t)   * p->count);
        p->new_stack         = realloc(p->new_stack,         sizeof(uint32_t)   * p->count);
        p->day_learning_heap = realloc(p->day_learning_heap, sizeof(uint32_t)   * p->count);
        p->capacity = p->count;
    }

    p->learning_heap_size     = 0;
    p->day_learning_heap_size = 0;
    p->review_heap_size       = 0;
    p->new_stack_size         = 0;

    uint64_t now   = srs_now();
    uint64_t today = srs_today(now);

    for (uint32_t i = 0; i < p->count; ++i) {
        uint32_t id = p->items[i].entry_id;
        if (id < p->dict_size)
            bitmap_set(p->bitmap, id);

        srs_item *it = &p->items[i];

        if (it->state == SRS_STATE_SUSPENDED)
            continue;

        if (it->state == SRS_STATE_NEW) {
            p->new_stack[p->new_stack_size++] = i;
            continue;
        }

        if (it->state == SRS_STATE_LEARNING ||
            it->state == SRS_STATE_RELEARNING) {
            uint64_t due_day = srs_today(it->due);
            if (due_day <= today)
                heap_push_learning(p, i);
            else
                heap_push_day_learning(p, i);
            continue;
        }

        if (it->state == SRS_STATE_REVIEW)
            heap_push_review(p, i);
    }

    shuffle_new_stack(p);
}

/* ─────────────────────────────────────────────
    Peek / pop due
   ───────────────────────────────────────────── */

srs_item *srs_peek_due(srs_profile *p, uint64_t now)
{
    if (!p) return NULL;

    srs_update_day_if_needed(p, now);
    srs_promote_day_learning(p, now);

    /* 1. Learning / relearning due now */
    if (p->learning_heap_size > 0) {
        uint32_t idx = p->learning_heap[0];
        if (p->items[idx].due <= now)
            return &p->items[idx];
    }

    /* 2. Review due now (respects daily limit) */
    if (p->review_heap_size > 0) {
        uint32_t idx = p->review_heap[0];
        if (p->items[idx].due <= now) {
            if (p->daily_review_limit == 0 ||
                p->review_served_today < p->daily_review_limit)
                return &p->items[idx];
        }
    }

    /* 3. New cards (respects daily limit) */
    if (p->new_stack_size > 0) {
        if (p->daily_new_limit == 0 ||
            p->new_served_today < p->daily_new_limit) {
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

    /* 1. Learning / relearning */
    if (p->learning_heap_size > 0) {
        uint32_t idx = p->learning_heap[0];
        if (p->items[idx].due <= now) {
            uint32_t item_index = heap_pop(p, p->learning_heap,
                                           &p->learning_heap_size);
            out->index = item_index;
            out->item  = &p->items[item_index];
            if (p->daily_review_limit != 0)
                p->review_served_today++;
            return true;
        }
    }

    /* 2. Review */
    if (p->review_heap_size > 0) {
        uint32_t idx = p->review_heap[0];
        if (p->items[idx].due <= now) {
            if (p->daily_review_limit != 0 &&
                p->review_served_today >= p->daily_review_limit)
                return false;
            uint32_t item_index = heap_pop(p, p->review_heap,
                                           &p->review_heap_size);
            out->index = item_index;
            out->item  = &p->items[item_index];
            if (p->daily_review_limit != 0)
                p->review_served_today++;
            return true;
        }
    }

    /* 3. New */
    if (p->new_stack_size > 0) {
        if (p->daily_new_limit != 0 &&
            p->new_served_today >= p->daily_new_limit)
            return false;
        uint32_t item_index = p->new_stack[p->new_stack_size - 1];
        p->new_stack_size--;
        out->index = item_index;
        out->item  = &p->items[item_index];
        if (p->daily_new_limit != 0)
            p->new_served_today++;
        return true;
    }

    return false;
}

/* ─────────────────────────────────────────────
    Requeue
   ───────────────────────────────────────────── */

void srs_requeue(srs_profile *p, uint32_t index)
{
    if (!p || index >= p->count) return;

    srs_item *it = &p->items[index];

    if (it->state == SRS_STATE_SUSPENDED)
        return;

    if (it->state == SRS_STATE_NEW) {
        if (p->new_stack_size == p->capacity) {
            p->capacity *= 2;
            p->new_stack = realloc(p->new_stack,
                                   sizeof(uint32_t) * p->capacity);
        }
        p->new_stack[p->new_stack_size++] = index;
        /* Insert at random position to keep shuffle property */
        if (p->new_stack_size > 1) {
            uint32_t j   = srs_rand_u32() % p->new_stack_size;
            uint32_t tmp = p->new_stack[p->new_stack_size - 1];
            p->new_stack[p->new_stack_size - 1] = p->new_stack[j];
            p->new_stack[j] = tmp;
        }
        return;
    }

    if (it->state == SRS_STATE_LEARNING ||
        it->state == SRS_STATE_RELEARNING) {
        uint64_t now   = srs_now();
        uint64_t today = srs_today(now);
        if (srs_today(it->due) <= today)
            heap_push_learning(p, index);
        else
            heap_push_day_learning(p, index);
        return;
    }

    if (it->state == SRS_STATE_REVIEW)
        heap_push_review(p, index);
}

/* ─────────────────────────────────────────────
    FSRS core
   ─────────────────────────────────────────────
    Parameters
      retention_target : desired retrievability  (0.9)
      difficulty range : [0.3, 1.8]
      stability        : days until retrievability drops to retention_target

    State transitions
      NEW         → first answer → LEARNING (step 0)
      LEARNING    → AGAIN/BARELY → stay at step 0
                  → HARD/GOOD/EASY/PERFECT → advance step
                  → last step passed → REVIEW
      REVIEW      → normalised quality < retention_target → RELEARNING
                  → otherwise → stay REVIEW, update S/D
      RELEARNING  → GOOD/EASY/PERFECT → back to REVIEW
                  → otherwise → repeat first relearning step
   ───────────────────────────────────────────── */

#define FSRS_RETENTION  0.9f
#define FSRS_DIFF_MIN   0.3f
#define FSRS_DIFF_MAX   1.8f
#define FSRS_DAY_SEC    86400ULL

/*
 * New stability after a successful review.
 *
 * Derivation:
 *   Retrievability after t days: R(t) = exp(ln(R_target) * t / S)
 *   We want R(S') = R_target  →  S' = -S / ln(R_target)
 *   i.e. S grows by factor 1/(-ln(R_target)) each good review,
 *   modulated by difficulty so harder cards grow more slowly:
 *
 *     S' = S * (-ln(R_target))^(1/D - 1)
 *
 *   Simplified practical form used here:
 *     S' = S * exp(-ln(R_target) / D)
 *
 *   Since R_target=0.9, ln(0.9)≈-0.1054, so -ln(R_target)≈+0.1054.
 *   With D=0.6: exp(0.1054/0.6) = exp(0.1757) ≈ 1.19  (grows ~19% each rep).
 *   With D=1.8: exp(0.1054/1.8) = exp(0.0586) ≈ 1.06  (grows ~6%  each rep).
 */
static float fsrs_next_stability(float stability, float difficulty)
{
    /* -ln(R_target) / D  →  positive multiplier > 1 */
    return fmaxf(1.0f,
                 stability * expf(-logf(FSRS_RETENTION) / difficulty));
}

/* Internal no-fuzz variant used by srs_predict_due */
static void srs_answer_fsrs_nofuzz(srs_profile *p,
                                   srs_item    *it,
                                   srs_quality  q,
                                   uint64_t     now)
{
    if (!it) return;

    /* --- NEW --- */
    if (it->state == SRS_STATE_NEW) {
        it->state      = SRS_STATE_LEARNING;
        it->step       = 0;
        it->stability  = 1.0f;
        it->difficulty = 0.6f;
        it->retention  = 1.0f;
        it->due        = now + p->learning_steps[0];
        return;
    }

    /* --- LEARNING / RELEARNING --- */
    if (it->state == SRS_STATE_LEARNING ||
        it->state == SRS_STATE_RELEARNING) {

        if (q <= SRS_BARELY) {               /* AGAIN or BARELY → reset */
            it->step = 0;
            it->due  = now + p->learning_steps[0];
            return;
        }

        uint32_t next_step = it->step + 1;
        if (next_step < p->learning_steps_count) {
            it->step = next_step;
            it->due  = now + p->learning_steps[it->step];
            return;
        }

        /* Last step passed → graduate to REVIEW */
        it->state     = SRS_STATE_REVIEW;
        it->reps      = 1;
        it->stability = (q >= SRS_EASY) ? 4.0f : 1.0f;
        it->difficulty = 0.6f;
        it->retention  = FSRS_RETENTION;
        it->due = now + (uint64_t)(it->stability * FSRS_DAY_SEC);
        return;
    }

    /* --- REVIEW --- */
    if (it->state == SRS_STATE_REVIEW) {
        /*
         * Retrievability en este momento:
         *   R(t) = exp(ln(R_target) * t / S)
         * donde t = tiempo desde que fue programada (now - (due - S*DAY)).
         * Aproximamos t = S (se revisa justo al vencer) → R ≈ R_target.
         * Si se revisa tarde (t > S), R < R_target. Si temprano, R > R_target.
         * Usamos el tiempo real transcurrido desde que venció.
         */
        uint64_t scheduled = it->due - (uint64_t)(it->stability * FSRS_DAY_SEC);
        float elapsed_days = (now >= scheduled)
            ? (float)(now - scheduled) / (float)FSRS_DAY_SEC
            : 0.0f;
        float current_retention = expf(
            logf(FSRS_RETENTION) * elapsed_days / fmaxf(it->stability, 0.001f));
        current_retention = clampf(current_retention, 0.0f, 1.0f);

        /*
         * Calidad normalizada al rango [0,1] solo para ajuste de dificultad.
         * AGAIN=0 → 0.0,  BARELY=1 → 0.2,  HARD=2 → 0.4,
         * GOOD=3  → 0.6,  EASY=4   → 0.8,  PERFECT=5 → 1.0
         */
        float q_norm = clampf((float)q / 5.0f, 0.0f, 1.0f);

        /*
         * Lapse: AGAIN o BARELY (q < SRS_HARD).
         */
        if (q < SRS_HARD) {
            it->lapses++;
            if (it->lapses >= LEECH_THRESHOLD)
                it->flags |= SRS_FLAG_LEECH;

            it->state     = SRS_STATE_RELEARNING;
            it->step      = 0;
            it->stability = fmaxf(1.0f, it->stability * 0.5f);
            it->retention = current_retention;
            it->interval  = (uint16_t)fminf(it->stability, 65535.0f);
            it->due = now + p->learning_steps[0];
            return;
        }

        /*
         * Buena respuesta (HARD, GOOD, EASY, PERFECT).
         * Ajuste de dificultad centrado en GOOD (q_norm=0.6):
         *   HARD (0.4) → D sube   (más difícil)
         *   GOOD (0.6) → D sin cambio
         *   EASY (0.8) → D baja
         */
        it->difficulty = clampf(
            it->difficulty + 0.1f * (0.6f - q_norm),
            FSRS_DIFF_MIN, FSRS_DIFF_MAX);

        it->stability = fsrs_next_stability(it->stability, it->difficulty);
        it->reps++;
        it->retention = current_retention;   /* retrievability real, no q_norm */
        it->interval  = (uint16_t)fminf(it->stability, 65535.0f);
        it->due = now + (uint64_t)(it->stability * FSRS_DAY_SEC);
        return;
    }

    /* --- RELEARNING (guard para llamadas directas a _nofuzz) --- */
    if (it->state == SRS_STATE_RELEARNING) {
        if (q >= SRS_HARD) {
            it->state    = SRS_STATE_REVIEW;
            it->interval = (uint16_t)fminf(it->stability, 65535.0f);
            it->due      = now + (uint64_t)(it->stability * FSRS_DAY_SEC);
        } else {
            it->step = 0;
            it->due  = now + p->learning_steps[0];
        }
    }
}

/* Public variant with fuzz */
void srs_answer_fsrs(srs_profile *p, srs_item *it,
                     srs_quality q, uint64_t now)
{
    if (!it) return;
    srs_answer_fsrs_nofuzz(p, it, q, now);

    /* Apply fuzz only to graduated REVIEW cards */
    if (it->state == SRS_STATE_REVIEW && it->due > now) {
        float fuzz    = fsrs_fuzz();
        float days    = (float)(it->due - now) / (float)FSRS_DAY_SEC;
        days         *= fuzz;
        if (days < 1.0f) days = 1.0f;
        it->due       = now + (uint64_t)(days * FSRS_DAY_SEC);
        it->stability = days;               /* keep in sync */
        it->interval  = (uint16_t)fminf(days, 65535.0f);
    }
}

/* ─────────────────────────────────────────────
    Public answer entry-point
   ───────────────────────────────────────────── */

void srs_answer(srs_profile *p, srs_item *it,
                srs_quality q, uint64_t now)
{
    if (!p || !it) return;
    if (it->state == SRS_STATE_SUSPENDED) return;

    srs_quality q_eff = (p->button_mode == SRS_BUTTONS_4)
                        ? map6to4(q)
                        : q;

    LOG_DEBUG("[CORE][SRS][SRS_ANSWER] item=%u q=%d→%d state=%d "
              "reps=%u stability=%.2f difficulty=%.2f\n",
              it->entry_id, q, q_eff, it->state,
              it->reps, it->stability, it->difficulty);

    for (int qual = 0; qual <= 5; ++qual) {
        uint64_t due_q = srs_predict_due(p, it, (srs_quality)qual, now);
        uint64_t delta = (due_q > now) ? (due_q - now) : 0;
        if (delta < FSRS_DAY_SEC)
            LOG_DEBUG("  predict q%d → %llus (%.4fd)\n", qual,
                      (unsigned long long)delta,
                      (double)delta / 86400.0);
        else
            LOG_DEBUG("  predict q%d → %.2fd\n", qual,
                      (double)delta / 86400.0);
    }

    srs_answer_fsrs(p, it, q_eff, now);
}

/* ─────────────────────────────────────────────
    Add / Remove
   ───────────────────────────────────────────── */

bool srs_add(srs_profile *p, uint32_t entry_id, uint64_t now)
{
    if (!p || entry_id >= p->dict_size || srs_contains(p, entry_id))
        return false;

    if (p->count == p->capacity) {
        p->capacity *= 2;
        p->items             = realloc(p->items,             p->capacity * sizeof(srs_item));
        p->review_heap       = realloc(p->review_heap,       p->capacity * sizeof(uint32_t));
        p->learning_heap     = realloc(p->learning_heap,     p->capacity * sizeof(uint32_t));
        p->new_stack         = realloc(p->new_stack,         p->capacity * sizeof(uint32_t));
        p->day_learning_heap = realloc(p->day_learning_heap, p->capacity * sizeof(uint32_t));
    }

    uint32_t  idx = p->count++;
    srs_item *it  = &p->items[idx];

    it->entry_id   = entry_id;
    it->due        = now;
    it->interval   = 0;
    it->reps       = 0;
    it->lapses     = 0;
    it->ease       = 2.5f;      /* unused but kept for binary compat */
    it->state      = SRS_STATE_NEW;
    it->step       = 0;
    it->flags      = 0;
    it->stability  = 1.0f;
    it->difficulty = 0.6f;
    it->retention  = 1.0f;

    bitmap_set(p->bitmap, entry_id);

    /* Push to new stack with random insertion */
    p->new_stack[p->new_stack_size++] = idx;
    if (p->new_stack_size > 1) {
        uint32_t j   = srs_rand_u32() % p->new_stack_size;
        uint32_t tmp = p->new_stack[p->new_stack_size - 1];
        p->new_stack[p->new_stack_size - 1] = p->new_stack[j];
        p->new_stack[j] = tmp;
    }

    return true;
}

bool srs_remove(srs_profile *p, uint32_t entry_id)
{
    if (!p || entry_id >= p->dict_size || !srs_contains(p, entry_id))
        return false;

    for (uint32_t i = 0; i < p->count; ++i) {
        if (p->items[i].entry_id != entry_id)
            continue;

        bitmap_clear(p->bitmap, entry_id);

        uint32_t last_idx = p->count - 1;
        if (i != last_idx) {
            p->items[i] = p->items[last_idx];

            /* Fix up heap/stack indices that pointed at last_idx */
            for (uint32_t h = 0; h < p->review_heap_size; ++h)
                if (p->review_heap[h] == last_idx) { p->review_heap[h] = i; break; }
            for (uint32_t h = 0; h < p->learning_heap_size; ++h)
                if (p->learning_heap[h] == last_idx) { p->learning_heap[h] = i; break; }
            for (uint32_t h = 0; h < p->new_stack_size; ++h)
                if (p->new_stack[h] == last_idx) { p->new_stack[h] = i; break; }
            for (uint32_t h = 0; h < p->day_learning_heap_size; ++h)
                if (p->day_learning_heap[h] == last_idx) { p->day_learning_heap[h] = i; break; }
        }

        p->count--;
        return true;
    }

    return false;
}

/* ─────────────────────────────────────────────
    Load / Save
   ───────────────────────────────────────────── */

bool srs_save(const srs_profile *p, const char *path)
{
    if (!p || !path) return false;

    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    if (fwrite(&p->dict_size, sizeof(p->dict_size), 1, fp) != 1) goto fail;
    if (fwrite(&p->count,     sizeof(p->count),     1, fp) != 1) goto fail;

    if (p->count > 0) {
        if (fwrite(p->items, sizeof(srs_item), p->count, fp) != p->count)
            goto fail;
    }

    uint32_t bitmap_bytes = (p->dict_size + 7) / 8;
    if (bitmap_bytes > 0) {
        if (fwrite(p->bitmap, 1, bitmap_bytes, fp) != bitmap_bytes)
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

    uint32_t dict_size = 0, count = 0;

    if (fread(&dict_size, sizeof(dict_size), 1, fp) != 1) goto fail;
    if (dict_size != expected_dict_size) goto fail;
    if (fread(&count, sizeof(count), 1, fp) != 1) goto fail;

    srs_free(p);
    if (!srs_init(p, dict_size)) goto fail;

    if (count > 0) {
        if (count > p->capacity) {
            p->items             = realloc(p->items,             sizeof(srs_item)  * count);
            p->review_heap       = realloc(p->review_heap,       sizeof(uint32_t)  * count);
            p->learning_heap     = realloc(p->learning_heap,     sizeof(uint32_t)  * count);
            p->new_stack         = realloc(p->new_stack,         sizeof(uint32_t)  * count);
            p->day_learning_heap = realloc(p->day_learning_heap, sizeof(uint32_t)  * count);
            p->capacity = count;
        }

        if (fread(p->items, sizeof(srs_item), count, fp) != count)
            goto fail;
        p->count = count;
    }

    uint32_t bitmap_bytes = (dict_size + 7) / 8;
    if (bitmap_bytes > 0) {
        if (fread(p->bitmap, 1, bitmap_bytes, fp) != bitmap_bytes)
            goto fail;
    }

    fclose(fp);
    srs_heapify(p);
    return true;
fail:
    fclose(fp);
    return false;
}

/* ─────────────────────────────────────────────
    Stats
   ───────────────────────────────────────────── */

void srs_compute_stats(const srs_profile *p, uint64_t now, srs_stats *out)
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

/* ─────────────────────────────────────────────
    Configuration
   ───────────────────────────────────────────── */

void srs_set_limits(srs_profile *p,
                    uint32_t daily_new_limit,
                    uint32_t daily_review_limit)
{
    if (!p) return;
    p->daily_new_limit    = daily_new_limit;
    p->daily_review_limit = daily_review_limit;
}

bool srs_set_learning_steps(srs_profile *p,
                            const uint64_t *steps,
                            uint32_t count)
{
    if (!p || !steps || count == 0) return false;

    uint64_t *tmp = realloc(p->learning_steps, sizeof(uint64_t) * count);
    if (!tmp) return false;

    p->learning_steps       = tmp;
    p->learning_steps_count = count;
    for (uint32_t i = 0; i < count; ++i)
        p->learning_steps[i] = steps[i];

    return true;
}

/* ─────────────────────────────────────────────
    Predict due (no side-effects)
   ───────────────────────────────────────────── */

uint64_t srs_predict_due(const srs_profile *p,
                         const srs_item    *it,
                         srs_quality        q,
                         uint64_t           now)
{
    if (!p || !it) return now;
    if (!p->learning_steps || p->learning_steps_count == 0) return now;

    srs_item tmp = *it;

    srs_quality q_eff = (p->button_mode == SRS_BUTTONS_4)
                        ? map6to4(q)
                        : q;

    srs_answer_fsrs_nofuzz((srs_profile *)p, &tmp, q_eff, now);

    /* Guardia: nunca devolver menos que now */
    return (tmp.due >= now) ? tmp.due : now;
}