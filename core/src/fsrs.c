/*
 * fsrs.c — FSRS-5 Spaced Repetition engine
 *
 * Implementation with:
 *  - due_day (day index) + due_ts (unix secs per card)
 *  - learning/relearning steps in seconds
 *  - heaps (due_heap, learn_heap) keep per-card heap positions for O(log n) removals
 *  - order_mode: MIXED / REVIEW_FIRST / NEW_FIRST
 *  - fix: reviews_per_day == 0 treated as unlimited in fsrs_next_for_session
 */

#include "fsrs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

/* file magic / version for persistence */
#define FSRS_MAGIC          0x53525346u
#define FSRS_FILE_VER       3u

/* heap macros */
#define HEAP_PARENT(i)      (((i) - 1) >> 1)
#define HEAP_LEFT(i)        (((i) << 1) + 1)
#define HEAP_RIGHT(i)       (((i) << 1) + 2)
#define NO_IDX              0xFFFFFFFFu

/* FSRS default weights */
static const float FSRS5_W[19] = {
    0.4072f, 1.1829f, 3.1262f, 7.2102f,
    7.2189f, 0.5316f, 1.0651f, 0.0589f,
    1.5330f, 0.14013f, 1.0070f, 1.9395f,
    0.1100f, 0.2900f, 2.2700f, 0.2500f,
    2.9898f, 0.5100f, 0.3400f
};

/* default steps (seconds) */
static uint32_t _default_learn_steps[]   = {60, 600};
static uint32_t _default_relearn_steps[] = {600};

/* ==================== private deck ==================== */
struct fsrs_deck {
    fsrs_params params;

    const uint32_t *_learn_steps;
    uint32_t _learn_steps_n;
    const uint32_t *_relearn_steps;
    uint32_t _relearn_steps_n;

    fsrs_card *cards;
    uint32_t count;
    uint32_t capacity;

    uint32_t *id_to_index;
    uint32_t id_map_size;
    uint8_t *bitmap;
    uint32_t bitmap_bits;

    /* heaps store card indices; ordered by due_ts asc */
    uint32_t *due_heap;
    uint32_t due_heap_size;
    uint32_t due_heap_cap;

    uint32_t *learn_heap;
    uint32_t learn_heap_size;
    uint32_t learn_heap_cap;

    uint32_t *new_queue;
    uint32_t new_queue_size;
    uint32_t new_queue_cap;

    uint64_t current_day;
    uint32_t new_today;
    uint32_t reviews_today;

    uint32_t streak_type;
    uint32_t streak_count;

    uint64_t rng_state;
};

/* ==================== PRNG ==================== */
static uint64_t _rng_next(fsrs_deck *d) {
    d->rng_state += 0x9e3779b97f4a7c15ULL;
    uint64_t z = d->rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void fsrs_seed(uint64_t seed) {
    (void)seed; /* optional global seed hook */
}

/* clampers */
static inline float _clamp_difficulty(float d) {
    if (d < FSRS_MIN_DIFFICULTY) return FSRS_MIN_DIFFICULTY;
    if (d > FSRS_MAX_DIFFICULTY) return FSRS_MAX_DIFFICULTY;
    return d;
}
static inline float _clamp_stability(float s) {
    if (s < FSRS_MIN_STABILITY) return FSRS_MIN_STABILITY;
    if (s > FSRS_MAX_STABILITY) return FSRS_MAX_STABILITY;
    return s;
}

/* ==================== heap comparison (by due_ts then id) ==================== */
static inline int _heap_less(const fsrs_deck *d, uint32_t a, uint32_t b) {
    const fsrs_card *ca = &d->cards[a];
    const fsrs_card *cb = &d->cards[b];
    if (ca->due_ts != cb->due_ts) return ca->due_ts < cb->due_ts;
    return ca->id < cb->id;
}

/* helper to swap and update per-card heap positions */
static inline void _heap_swap_and_update(fsrs_deck *d, uint32_t *heap, uint32_t i, uint32_t j, bool is_learn) {
    uint32_t ai = heap[i];
    uint32_t aj = heap[j];
    heap[i] = aj;
    heap[j] = ai;
    if (is_learn) {
        d->cards[ai].heap_pos_learn = (int32_t)j;
        d->cards[aj].heap_pos_learn = (int32_t)i;
    } else {
        d->cards[ai].heap_pos_due = (int32_t)j;
        d->cards[aj].heap_pos_due = (int32_t)i;
    }
}

static void _heap_sift_up(fsrs_deck *d, uint32_t *heap, uint32_t size, bool is_learn) {
    if (size == 0) return;
    uint32_t i = size - 1;
    while (i > 0) {
        uint32_t p = HEAP_PARENT(i);
        if (_heap_less(d, heap[i], heap[p])) {
            _heap_swap_and_update(d, heap, i, p, is_learn);
            i = p;
        } else break;
    }
}

static void _heap_sift_down(fsrs_deck *d, uint32_t *heap, uint32_t size, uint32_t i, bool is_learn) {
    for (;;) {
        uint32_t smallest = i;
        uint32_t l = HEAP_LEFT(i);
        uint32_t r = HEAP_RIGHT(i);
        if (l < size && _heap_less(d, heap[l], heap[smallest])) smallest = l;
        if (r < size && _heap_less(d, heap[r], heap[smallest])) smallest = r;
        if (smallest == i) break;
        _heap_swap_and_update(d, heap, i, smallest, is_learn);
        i = smallest;
    }
}

/* push element to heap; maintain per-card heap_pos_* */
static bool _heap_push(fsrs_deck *d, uint32_t **heap, uint32_t *size, uint32_t *cap, uint32_t idx, bool is_learn) {
    if (*size == *cap) {
        uint32_t nc = (*cap) ? (*cap) * 2 : 64;
        uint32_t *nh = (uint32_t*)realloc(*heap, nc * sizeof(uint32_t));
        if (!nh) return false;
        *heap = nh;
        *cap = nc;
    }
    uint32_t pos = (*size);
    (*heap)[pos] = idx;
    if (is_learn) d->cards[idx].heap_pos_learn = (int32_t)pos;
    else d->cards[idx].heap_pos_due = (int32_t)pos;
    (*size)++;
    _heap_sift_up(d, *heap, *size, is_learn);
    return true;
}

/* pop top element from heap; update last element pos and clear popped pos */
static uint32_t _heap_pop(fsrs_deck *d, uint32_t *heap, uint32_t *size, bool is_learn) {
    uint32_t top = heap[0];
    uint32_t last = heap[--(*size)];
    if (*size > 0) {
        heap[0] = last;
        if (is_learn) d->cards[last].heap_pos_learn = 0;
        else d->cards[last].heap_pos_due = 0;
        _heap_sift_down(d, heap, *size, 0, is_learn);
    }
    if (is_learn) d->cards[top].heap_pos_learn = -1;
    else d->cards[top].heap_pos_due = -1;
    return top;
}

/* remove by card index using stored heap_pos_* (O(log n)) */
static void _heap_remove(fsrs_deck *d, uint32_t *heap, uint32_t *size, uint32_t idx, bool is_learn) {
    int32_t pos = is_learn ? d->cards[idx].heap_pos_learn : d->cards[idx].heap_pos_due;
    if (pos < 0 || (uint32_t)pos >= *size) return;
    uint32_t last = heap[--(*size)];
    if ((uint32_t)pos < *size) {
        heap[pos] = last;
        if (is_learn) d->cards[last].heap_pos_learn = pos;
        else d->cards[last].heap_pos_due = pos;
        if (pos > 0 && _heap_less(d, heap[pos], heap[HEAP_PARENT(pos)]))
            _heap_sift_up(d, heap, pos + 1, is_learn);
        else
            _heap_sift_down(d, heap, *size, pos, is_learn);
    }
    if (is_learn) d->cards[idx].heap_pos_learn = -1;
    else d->cards[idx].heap_pos_due = -1;
}

/* new queue helpers */
static bool _newq_push(fsrs_deck *d, uint32_t idx) {
    if (d->new_queue_size == d->new_queue_cap) {
        uint32_t nc = d->new_queue_cap ? d->new_queue_cap * 2 : 64;
        uint32_t *nq = (uint32_t*)realloc(d->new_queue, nc * sizeof(uint32_t));
        if (!nq) return false;
        d->new_queue = nq;
        d->new_queue_cap = nc;
    }
    d->new_queue[d->new_queue_size++] = idx;
    return true;
}

static void _newq_shuffle(fsrs_deck *d) {
    uint32_t n = d->new_queue_size;
    if (n < 2) return;
    for (uint32_t i = n - 1; i > 0; i--) {
        uint32_t j = (uint32_t)(_rng_next(d) % (uint64_t)(i + 1));
        uint32_t t = d->new_queue[i];
        d->new_queue[i] = d->new_queue[j];
        d->new_queue[j] = t;
    }
}

/* id map / bitmap helpers */
static bool _idmap_ensure(fsrs_deck *d, uint32_t id) {
    if (id < d->id_map_size) return true;
    uint32_t nc = d->id_map_size ? d->id_map_size * 2 : 256;
    while (nc <= id) nc *= 2;
    uint32_t *nm = (uint32_t*)realloc(d->id_to_index, nc * sizeof(uint32_t));
    if (!nm) return false;
    for (uint32_t i = d->id_map_size; i < nc; i++) nm[i] = NO_IDX;
    d->id_to_index = nm;
    d->id_map_size = nc;
    return true;
}

static bool _bitmap_ensure(fsrs_deck *d, uint32_t id) {
    if (id < d->bitmap_bits) return true;
    uint32_t nb = d->bitmap_bits ? d->bitmap_bits * 2 : 512;
    while (nb <= id) nb *= 2;
    uint8_t *bm = (uint8_t*)realloc(d->bitmap, nb / 8);
    if (!bm) return false;
    memset(bm + d->bitmap_bits / 8, 0, (nb - d->bitmap_bits) / 8);
    d->bitmap = bm;
    d->bitmap_bits = nb;
    return true;
}

static void _bitmap_set(fsrs_deck *d, uint32_t id, bool v) {
    if (v) d->bitmap[id >> 3] |= (uint8_t)(1u << (id & 7));
    else d->bitmap[id >> 3] &= ~(uint8_t)(1u << (id & 7));
}

/* ==================== day helpers ==================== */
uint64_t fsrs_current_day(fsrs_deck *d, uint64_t now) {
    uint32_t off = d->params.day_offset_secs;
    if (now < off) return 0;
    return (now - off) / FSRS_SEC_PER_DAY;
}

uint64_t fsrs_day_start(const fsrs_deck *d, uint64_t day) {
    return day * FSRS_SEC_PER_DAY + d->params.day_offset_secs;
}

static void _maybe_new_day(fsrs_deck *d, uint64_t now) {
    uint64_t today = fsrs_current_day(d, now);
    if (today != d->current_day) {
        d->current_day = today;
        d->new_today = 0;
        d->reviews_today = 0;
        d->streak_count = 0;
        d->streak_type  = 0;
    }
}

/* fuzz helper */
static uint32_t _apply_fuzz(fsrs_deck *d, uint32_t interval) {
    if (interval < 7) return interval;
    uint32_t r = (uint32_t)(_rng_next(d) % 11);
    float factor = 0.95f + (r / 100.0f);
    uint32_t varied = (uint32_t)(interval * factor);
    if (varied < interval - 2) varied = interval - 2;
    if (varied > interval + 2) varied = interval + 2;
    if (varied < 1) varied = 1;
    return varied;
}

/* ==================== FSRS math helpers ==================== */
static inline float _s0(const float w[19], fsrs_rating r) {
    int ri = (int)r - 1;
    if (ri < 0) ri = 0;
    if (ri > 3) ri = 3;
    return fmaxf(w[ri], 0.1f);
}

static inline float _d0(const float w[19], fsrs_rating r) {
    float d = w[4] - expf(w[5] * (float)((int)r - 1)) + 1.0f;
    return _clamp_difficulty(d);
}

static inline float _s_short(const float w[19], float s, fsrs_rating r) {
    float factor = expf(w[17] * ((float)((int)r - 3) + w[18]));
    return _clamp_stability(s * factor);
}

static float _s_recall(const float w[19], float d, float s, float r_val, fsrs_rating rating) {
    float hard_penalty = (rating == FSRS_HARD) ? w[15] : 1.0f;
    float easy_bonus   = (rating == FSRS_EASY) ? w[16] : 1.0f;
    float new_s = s * (expf(w[8]) * (11.0f - d) * powf(s, -w[9])
                       * (expf(w[10] * (1.0f - r_val)) - 1.0f)
                       * hard_penalty * easy_bonus + 1.0f);
    return _clamp_stability(new_s);
}

static float _s_forget(const float w[19], float d, float s, float r_val) {
    float new_s = w[11] * powf(d, -w[12]) * (powf(s + 1.0f, w[13]) - 1.0f)
                  * expf(w[14] * (1.0f - r_val));
    return _clamp_stability(new_s);
}

static float _d_update(const float w[19], float d, fsrs_rating r) {
    float d_target = _d0(w, FSRS_EASY);
    float delta = -w[6] * (float)((int)r - 3);
    float new_d = w[7] * d_target + (1.0f - w[7]) * (d + delta);
    return _clamp_difficulty(new_d);
}

static uint32_t fsrs_fuzz_days(fsrs_deck *deck, uint32_t ivl)
{
    if (!deck->params.enable_fuzz)
        return ivl;

    if (ivl < 2)
        return ivl;

    float fuzz_factor;
    if (ivl < 7) fuzz_factor = 0.15f;
    else if (ivl < 30) fuzz_factor = 0.10f;
    else fuzz_factor = 0.05f;

    int fuzz = (int)(ivl * fuzz_factor);
    if (fuzz < 1) fuzz = 1;

    int min = (int)ivl - fuzz;
    int max = (int)ivl + fuzz;
    if (min < 1) min = 1;

    uint32_t r = (uint32_t)(_rng_next(deck) % (uint64_t)(max - min + 1));
    return (uint32_t)(min + r);
}

static void fsrs_normalize_intervals(uint32_t ivl[4])
{
    if(ivl[1] <= ivl[0]) ivl[1] = ivl[0] + 1;
    if(ivl[2] <= ivl[1]) ivl[2] = ivl[1] + 1;
    if(ivl[3] <= ivl[2]) ivl[3] = ivl[2] + 1;
}

static void fsrs_normalize_seconds(uint64_t out[4], uint64_t now)
{
    uint64_t again = out[0] - now;
    uint64_t hard  = out[1] - now;
    uint64_t good  = out[2] - now;
    uint64_t easy  = out[3] - now;

    if(hard <= again) { hard = again + 60; out[1] = now + hard; }
    if(good <= hard)  { good = hard  + 60; out[2] = now + good; }
    if(easy <= good)  { easy = good  + 60; out[3] = now + easy; }
}

static uint32_t _next_interval(fsrs_deck *d, float s, bool apply_fuzz) {
    if (s <= 0.0f) return 1;
    float i = (float)(9.0 * (double)s * ((1.0 / (double)d->params.desired_retention) - 1.0));
    uint32_t days = (uint32_t)(i + 0.5f);
    if (days < 1) days = 1;
    if (days > d->params.maximum_interval) days = d->params.maximum_interval;
    if (apply_fuzz && d->params.enable_fuzz) days = _apply_fuzz(d, days);
    return days;
}

/* ==================== Lifecycle ==================== */

fsrs_params fsrs_default_params(void) {
    fsrs_params p;
    memset(&p, 0, sizeof(p));
    memcpy(p.w, FSRS5_W, sizeof(FSRS5_W));
    p.desired_retention      = 0.9f;
    p.maximum_interval       = 36500;
    p.leech_threshold        = 8;
    p.day_offset_secs        = FSRS_DEFAULT_OFFSET;
    p.new_cards_per_day      = 20;
    p.reviews_per_day        = 200;
    p.learning_steps         = _default_learn_steps;
    p.learning_steps_count   = 2;
    p.relearning_steps       = _default_relearn_steps;
    p.relearning_steps_count = 1;
    p.new_review_ratio       = 0.5f;
    p.mix_burst_size         = 5;
    p.enable_fuzz            = true;
    p.order_mode             = FSRS_ORDER_MIXED;
    return p;
}

fsrs_deck* fsrs_deck_create(uint32_t id_space, const fsrs_params *params) {
    fsrs_deck *d = (fsrs_deck*)calloc(1, sizeof(fsrs_deck));
    if (!d) return NULL;
    d->params = params ? *params : fsrs_default_params();
    d->_learn_steps   = d->params.learning_steps   ? d->params.learning_steps   : _default_learn_steps;
    d->_learn_steps_n = d->params.learning_steps_count ? d->params.learning_steps_count : 2;
    d->_relearn_steps   = d->params.relearning_steps   ? d->params.relearning_steps   : _default_relearn_steps;
    d->_relearn_steps_n = d->params.relearning_steps_count ? d->params.relearning_steps_count : 1;

    uint32_t bits = id_space ? id_space : 512;
    if (!_bitmap_ensure(d, bits - 1) || !_idmap_ensure(d, bits - 1)) {
        fsrs_deck_free(d);
        return NULL;
    }

    d->rng_state  = (uint64_t)time(NULL) ^ 0x9e3779b97f4a7c15ULL;
    d->current_day = fsrs_current_day(d, (uint64_t)time(NULL));
    return d;
}

void fsrs_deck_free(fsrs_deck *d) {
    if (!d) return;
    free(d->cards);
    free(d->bitmap);
    free(d->id_to_index);
    free(d->due_heap);
    free(d->learn_heap);
    free(d->new_queue);
    free(d);
}

/* ==================== Card management ==================== */

fsrs_card* fsrs_add_card(fsrs_deck *d, uint32_t id, uint64_t now) {
    if (!d) return NULL;
    if (fsrs_get_card(d, id)) return NULL;
    if (!_bitmap_ensure(d, id) || !_idmap_ensure(d, id)) return NULL;

    if (d->count == d->capacity) {
        uint32_t nc = d->capacity ? d->capacity * 2 : 64;
        fsrs_card *na = (fsrs_card*)realloc(d->cards, nc * sizeof(fsrs_card));
        if (!na) return NULL;
        d->cards = na;
        d->capacity = nc;
    }

    uint32_t idx = d->count++;
    fsrs_card *c = &d->cards[idx];
    memset(c, 0, sizeof(*c));
    c->id         = id;
    c->state      = FSRS_STATE_NEW;
    c->stability  = 0.0f;
    c->difficulty = FSRS_MIN_DIFFICULTY;
    c->reps       = 0;
    c->lapses     = 0;
    c->total_answers = 0;
    c->step       = 0;
    c->flags      = FSRS_FLAG_NONE;
    c->heap_pos_due   = -1;
    c->heap_pos_learn = -1;
    c->due_ts  = now;
    c->due_day = (uint32_t)fsrs_current_day(d, now);
    c->last_review = 0;
    c->prev_state  = 0;

    _bitmap_set(d, id, true);
    d->id_to_index[id] = idx;
    _newq_push(d, idx);
    return c;
}

bool fsrs_remove_card(fsrs_deck *d, uint32_t id) {
    if (!d || !fsrs_get_card(d, id)) return false;
    uint32_t idx = d->id_to_index[id];
    fsrs_card *c = &d->cards[idx];

    switch (c->state) {
        case FSRS_STATE_NEW:
            for (uint32_t i = 0; i < d->new_queue_size; i++) {
                if (d->new_queue[i] == idx) {
                    d->new_queue[i] = d->new_queue[--d->new_queue_size];
                    break;
                }
            }
            break;
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            _heap_remove(d, d->learn_heap, &d->learn_heap_size, idx, true);
            break;
        case FSRS_STATE_REVIEW:
            _heap_remove(d, d->due_heap, &d->due_heap_size, idx, false);
            break;
        default: break;
    }

    _bitmap_set(d, id, false);
    d->id_to_index[id] = NO_IDX;

    uint32_t last = d->count - 1;
    if (idx != last) {
        d->cards[idx] = d->cards[last];
        uint32_t moved_id = d->cards[idx].id;
        d->id_to_index[moved_id] = idx;

        if (d->cards[idx].heap_pos_due >= 0 && (uint32_t)d->cards[idx].heap_pos_due < d->due_heap_size)
            d->due_heap[d->cards[idx].heap_pos_due] = idx;
        if (d->cards[idx].heap_pos_learn >= 0 && (uint32_t)d->cards[idx].heap_pos_learn < d->learn_heap_size)
            d->learn_heap[d->cards[idx].heap_pos_learn] = idx;
        for (uint32_t i = 0; i < d->new_queue_size; i++)
            if (d->new_queue[i] == last) { d->new_queue[i] = idx; break; }
    }
    d->count--;
    return true;
}

fsrs_card* fsrs_get_card(fsrs_deck *d, uint32_t id) {
    if (!d || id >= d->id_map_size) return NULL;
    uint32_t idx = d->id_to_index[id];
    if (idx == NO_IDX || idx >= d->count) return NULL;
    return &d->cards[idx];
}

void fsrs_suspend(fsrs_deck *d, uint32_t id) {
    fsrs_card *c = fsrs_get_card(d, id);
    if (!c || c->state == FSRS_STATE_SUSPENDED) return;
    uint32_t idx = d->id_to_index[id];

    c->prev_state = c->state;

    switch (c->state) {
        case FSRS_STATE_NEW:
            for (uint32_t i = 0; i < d->new_queue_size; i++) {
                if (d->new_queue[i] == idx) {
                    d->new_queue[i] = d->new_queue[--d->new_queue_size];
                    break;
                }
            }
            break;
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            _heap_remove(d, d->learn_heap, &d->learn_heap_size, idx, true);
            break;
        case FSRS_STATE_REVIEW:
            _heap_remove(d, d->due_heap, &d->due_heap_size, idx, false);
            break;
        default: break;
    }

    c->state = FSRS_STATE_SUSPENDED;
}

void fsrs_unsuspend(fsrs_deck *d, uint32_t id, uint64_t now) {
    fsrs_card *c = fsrs_get_card(d, id);
    if (!c || c->state != FSRS_STATE_SUSPENDED) return;
    uint32_t idx = d->id_to_index[id];
    uint32_t today = (uint32_t)fsrs_current_day(d, now);

    uint8_t prev = c->prev_state;

    switch (prev) {
        case FSRS_STATE_NEW:
            c->state   = FSRS_STATE_NEW;
            c->due_ts  = now;
            c->due_day = today;
            _newq_push(d, idx);
            break;

        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            c->state = (fsrs_state)prev;
            _heap_push(d, &d->learn_heap, &d->learn_heap_size,
                       &d->learn_heap_cap, idx, true);
            break;

        case FSRS_STATE_REVIEW:
        default:
            c->state   = FSRS_STATE_REVIEW;
            c->due_day = today;
            c->due_ts  = fsrs_day_start(d, today);
            _heap_push(d, &d->due_heap, &d->due_heap_size,
                       &d->due_heap_cap, idx, false);
            break;
    }

    c->prev_state = 0;
}

void fsrs_mark(fsrs_deck *d, uint32_t id, bool marked) {
    fsrs_card *c = fsrs_get_card(d, id);
    if (!c) return;
    if (marked) c->flags |= FSRS_FLAG_MARKED; else c->flags &= ~FSRS_FLAG_MARKED;
}

void fsrs_snooze(fsrs_deck *d, uint32_t id, uint32_t days, uint64_t now) {
    fsrs_card *c = fsrs_get_card(d, id);
    if (!c || c->state != FSRS_STATE_REVIEW) return;
    uint32_t idx = d->id_to_index[id];
    _heap_remove(d, d->due_heap, &d->due_heap_size, idx, false);
    uint32_t target_day = (uint32_t)(fsrs_current_day(d, now) + days);
    c->due_day = target_day;
    c->due_ts  = fsrs_day_start(d, target_day);
    c->flags  |= FSRS_FLAG_SNOOZED;
    _heap_push(d, &d->due_heap, &d->due_heap_size, &d->due_heap_cap, idx, false);
}

/* ==================== QUEUE MANAGEMENT ==================== */

void fsrs_rebuild_queues(fsrs_deck *d, uint64_t now) {
    if (!d) return;
    d->due_heap_size  = 0;
    d->learn_heap_size = 0;
    d->new_queue_size  = 0;
    d->streak_count    = 0;
    d->streak_type     = 0;
    _maybe_new_day(d, now);
    uint32_t today = (uint32_t)d->current_day;

    for (uint32_t i = 0; i < d->count; i++) {
        fsrs_card *c = &d->cards[i];
        if (c->due_day <= today) c->flags |= FSRS_FLAG_DUE;
        else c->flags &= ~FSRS_FLAG_DUE;

        c->heap_pos_due   = -1;
        c->heap_pos_learn = -1;

        switch (c->state) {
            case FSRS_STATE_NEW:
                _newq_push(d, i);
                break;
            case FSRS_STATE_LEARNING:
            case FSRS_STATE_RELEARNING:
                _heap_push(d, &d->learn_heap, &d->learn_heap_size, &d->learn_heap_cap, i, true);
                break;
            case FSRS_STATE_REVIEW:
                _heap_push(d, &d->due_heap, &d->due_heap_size, &d->due_heap_cap, i, false);
                break;
            default: break;
        }
    }

    _newq_shuffle(d);
}

/* ==================== SCHEDULING CORE ==================== */

/*
 * _next_card_internal — shared logic for fsrs_next_card_ex and fsrs_next_for_session.
 *
 * Priority:
 *   1. Learning / relearning ready (due_ts <= now)          — always first
 *   2. Reviews / new according to order_mode
 *   3. Earliest pending learning (due_ts > now)             — fallback
 *   4. Earliest review (future day)                         — fallback
 */
static fsrs_card* _next_card_internal(fsrs_deck *d, uint64_t now, fsrs_queue_type *out_queue) {
    _maybe_new_day(d, now);
    uint32_t today = (uint32_t)d->current_day;

    /* 1 — learning / relearning ready */
    if (d->learn_heap_size > 0) {
        uint32_t idx = d->learn_heap[0];
        if (d->cards[idx].due_ts <= now) {
            if (out_queue) *out_queue = FSRS_QUEUE_LEARN;
            return &d->cards[idx];
        }
    }

    /* limits — 0 means unlimited */
    uint32_t rlimit = d->params.reviews_per_day;
    bool review_ok  = (rlimit == 0) || (d->reviews_today < rlimit);
    uint32_t nlimit = d->params.new_cards_per_day;
    bool new_ok     = (nlimit == 0) || (d->new_today < nlimit);

    bool has_review = review_ok && d->due_heap_size > 0 &&
                      d->cards[d->due_heap[0]].due_day <= today;
    bool has_new    = new_ok && d->new_queue_size > 0;

    switch (d->params.order_mode) {

        case FSRS_ORDER_REVIEW_FIRST:
            if (has_review) {
                if (out_queue) *out_queue = FSRS_QUEUE_REVIEW;
                return &d->cards[d->due_heap[0]];
            }
            if (has_new) {
                if (out_queue) *out_queue = FSRS_QUEUE_NEW;
                return &d->cards[d->new_queue[0]];
            }
            break;

        case FSRS_ORDER_NEW_FIRST:
            if (has_new) {
                if (out_queue) *out_queue = FSRS_QUEUE_NEW;
                return &d->cards[d->new_queue[0]];
            }
            if (has_review) {
                if (out_queue) *out_queue = FSRS_QUEUE_REVIEW;
                return &d->cards[d->due_heap[0]];
            }
            break;

        case FSRS_ORDER_MIXED:
        default:
            if (has_review && has_new) {
                /* interleave: serve a new card every mix_burst_size reviews */
                uint32_t burst = d->params.mix_burst_size ? d->params.mix_burst_size : 5;
                if (d->streak_type == FSRS_QUEUE_REVIEW && d->streak_count >= burst) {
                    if (out_queue) *out_queue = FSRS_QUEUE_NEW;
                    return &d->cards[d->new_queue[0]];
                }
                if (out_queue) *out_queue = FSRS_QUEUE_REVIEW;
                return &d->cards[d->due_heap[0]];
            }
            if (has_review) {
                if (out_queue) *out_queue = FSRS_QUEUE_REVIEW;
                return &d->cards[d->due_heap[0]];
            }
            if (has_new) {
                if (out_queue) *out_queue = FSRS_QUEUE_NEW;
                return &d->cards[d->new_queue[0]];
            }
            break;
    }

    /* 3 — earliest pending learning (not yet ready) */
    if (d->learn_heap_size > 0) {
        if (out_queue) *out_queue = FSRS_QUEUE_DAY_LEARN;
        return &d->cards[d->learn_heap[0]];
    }

    /* 4 — earliest review (future day) */
    if (d->due_heap_size > 0) {
        if (out_queue) *out_queue = FSRS_QUEUE_REVIEW;
        return &d->cards[d->due_heap[0]];
    }

    if (out_queue) *out_queue = FSRS_QUEUE_NONE;
    return NULL;
}

fsrs_card* fsrs_next_card(fsrs_deck *d, uint64_t now) {
    fsrs_queue_type tmp;
    return fsrs_next_card_ex(d, now, &tmp);
}

fsrs_card* fsrs_next_card_ex(fsrs_deck *d, uint64_t now, fsrs_queue_type *out_queue) {
    if (!d) return NULL;
    return _next_card_internal(d, now, out_queue);
}

void fsrs_session_start(fsrs_session *s, uint64_t now, uint32_t limit) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->session_start = now;
    s->session_limit = limit;
}

fsrs_card* fsrs_next_for_session(fsrs_deck *d, fsrs_session *s, uint64_t now) {
    if (!d || !s) return NULL;
    if (s->session_limit > 0 && s->cards_done >= s->session_limit) return NULL;
    return _next_card_internal(d, now, NULL);
}

uint64_t fsrs_wait_seconds(fsrs_deck *d, uint64_t now) {
    if (!d) return 0;
    _maybe_new_day(d, now);

    if (fsrs_next_card(d, now)) return 0;

    uint64_t earliest = UINT64_MAX;

    if (d->learn_heap_size > 0) {
        uint32_t idx = d->learn_heap[0];
        if (d->cards[idx].due_ts > now && d->cards[idx].due_ts < earliest)
            earliest = d->cards[idx].due_ts;
    }

    if (d->due_heap_size > 0) {
        uint32_t idx = d->due_heap[0];
        if (d->cards[idx].due_ts > now && d->cards[idx].due_ts < earliest)
            earliest = d->cards[idx].due_ts;
    }

    if (earliest == UINT64_MAX || earliest <= now) return 0;
    return earliest - now;
}

/* ==================== ENQUEUE helper ==================== */
static void _enqueue(fsrs_deck *d, uint32_t idx) {
    fsrs_card *c = &d->cards[idx];
    uint32_t today = (uint32_t)d->current_day;
    if (c->due_day <= today) c->flags |= FSRS_FLAG_DUE;
    else c->flags &= ~FSRS_FLAG_DUE;

    if (c->state == FSRS_STATE_LEARNING || c->state == FSRS_STATE_RELEARNING) {
        _heap_push(d, &d->learn_heap, &d->learn_heap_size, &d->learn_heap_cap, idx, true);
    } else if (c->state == FSRS_STATE_REVIEW) {
        _heap_push(d, &d->due_heap, &d->due_heap_size, &d->due_heap_cap, idx, false);
    }
}

/* ==================== Scheduling: fsrs_answer ==================== */

void fsrs_answer(fsrs_deck *d, fsrs_card *card, fsrs_rating rating, uint64_t now) {
    if (!d || !card) return;
    _maybe_new_day(d, now);

    const float *w = d->params.w;
    uint32_t idx   = (uint32_t)(card - d->cards);
    uint32_t today = (uint32_t)fsrs_current_day(d, now);

    uint64_t elapsed_secs = 0;
    if (card->last_review > 0 && now > card->last_review) elapsed_secs = now - card->last_review;
    float r_val = fsrs_retrievability(card->stability, elapsed_secs);

    /* remove from current queue */
    fsrs_queue_type prev_queue = FSRS_QUEUE_NONE;
    switch (card->state) {
        case FSRS_STATE_NEW:
            for (uint32_t i = 0; i < d->new_queue_size; i++) {
                if (d->new_queue[i] == idx) {
                    d->new_queue[i] = d->new_queue[--d->new_queue_size];
                    break;
                }
            }
            d->new_today++;
            prev_queue = FSRS_QUEUE_NEW;
            break;
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            _heap_remove(d, d->learn_heap, &d->learn_heap_size, idx, true);
            prev_queue = FSRS_QUEUE_LEARN;
            break;
        case FSRS_STATE_REVIEW:
            _heap_remove(d, d->due_heap, &d->due_heap_size, idx, false);
            d->reviews_today++;
            prev_queue = FSRS_QUEUE_REVIEW;
            break;
        default: break;
    }

    /* update streak for MIXED mode */
    if (d->params.order_mode == FSRS_ORDER_MIXED) {
        if (prev_queue == FSRS_QUEUE_REVIEW || prev_queue == FSRS_QUEUE_LEARN) {
            if (d->streak_type == FSRS_QUEUE_REVIEW) {
                d->streak_count++;
            } else {
                d->streak_type  = FSRS_QUEUE_REVIEW;
                d->streak_count = 1;
            }
        } else if (prev_queue == FSRS_QUEUE_NEW) {
            if (d->streak_type == FSRS_QUEUE_NEW) {
                d->streak_count++;
            } else {
                d->streak_type  = FSRS_QUEUE_NEW;
                d->streak_count = 1;
            }
        }
    }

    card->last_review  = now;
    card->total_answers++;
    card->flags &= ~FSRS_FLAG_SNOOZED;

    /* state machine */
    switch (card->state) {
        case FSRS_STATE_NEW:
            card->stability  = _s0(w, rating);
            card->difficulty = _d0(w, rating);
            card->reps   = 0;
            card->lapses = 0;
            card->step   = 0;

            if (rating == FSRS_AGAIN) {
                card->state = FSRS_STATE_LEARNING;
                card->step  = 0;
                uint32_t secs = d->_learn_steps_n ? d->_learn_steps[0] : 60;
                card->due_ts  = now + (uint64_t)secs;
                card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
            } else if (d->_learn_steps_n == 0 || rating == FSRS_EASY) {
                card->state   = FSRS_STATE_REVIEW;
                uint32_t days = _next_interval(d, card->stability, true);
                card->due_day = today + days;
                card->due_ts  = fsrs_day_start(d, card->due_day);
                card->reps    = 1;
            } else {
                uint32_t step = (rating == FSRS_GOOD) ? 1 : 0;
                if (step >= d->_learn_steps_n) {
                    card->state   = FSRS_STATE_REVIEW;
                    uint32_t days = _next_interval(d, card->stability, true);
                    card->due_day = today + days;
                    card->due_ts  = fsrs_day_start(d, card->due_day);
                    card->reps    = 1;
                } else {
                    card->state   = FSRS_STATE_LEARNING;
                    card->step    = (uint8_t)step;
                    uint32_t secs = d->_learn_steps[card->step];
                    card->due_ts  = now + (uint64_t)secs;
                    card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
                }
            }
            break;

        case FSRS_STATE_LEARNING: {
            card->stability  = _s_short(w, card->stability, rating);
            card->difficulty = _d_update(w, card->difficulty, rating);

            if (rating == FSRS_AGAIN) {
                card->step    = 0;
                uint32_t secs = d->_learn_steps_n ? d->_learn_steps[0] : 60;
                card->due_ts  = now + (uint64_t)secs;
                card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
            } else if (rating == FSRS_EASY) {
                card->state   = FSRS_STATE_REVIEW;
                card->reps++;
                uint32_t days = _next_interval(d, card->stability, true);
                card->due_day = today + days;
                card->due_ts  = fsrs_day_start(d, card->due_day);
            } else {
                uint32_t next_step = card->step + (rating >= FSRS_GOOD ? 1 : 0);
                if (next_step >= d->_learn_steps_n) {
                    card->state   = FSRS_STATE_REVIEW;
                    card->reps++;
                    uint32_t days = _next_interval(d, card->stability, true);
                    card->due_day = today + days;
                    card->due_ts  = fsrs_day_start(d, card->due_day);
                } else {
                    card->step    = (uint8_t)next_step;
                    uint32_t secs = d->_learn_steps[card->step];
                    card->due_ts  = now + (uint64_t)secs;
                    card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
                }
            }
            break;
        }

        case FSRS_STATE_REVIEW:
            if (rating == FSRS_AGAIN) {
                card->lapses++;
                card->stability  = _s_forget(w, card->difficulty, card->stability, r_val);
                card->difficulty = _d_update(w, card->difficulty, rating);
                if (d->params.leech_threshold > 0 && card->lapses >= d->params.leech_threshold)
                    card->flags |= FSRS_FLAG_LEECH;
                if (d->_relearn_steps_n > 0) {
                    card->state   = FSRS_STATE_RELEARNING;
                    card->step    = 0;
                    uint32_t secs = d->_relearn_steps[0];
                    card->due_ts  = now + (uint64_t)secs;
                    card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
                } else {
                    uint32_t days = _next_interval(d, card->stability, true);
                    card->due_day = today + days;
                    card->due_ts  = fsrs_day_start(d, card->due_day);
                }
            } else {
                card->stability  = _s_recall(w, card->difficulty, card->stability, r_val, rating);
                card->difficulty = _d_update(w, card->difficulty, rating);
                card->reps++;
                uint32_t days = _next_interval(d, card->stability, true);
                card->due_day = today + days;
                card->due_ts  = fsrs_day_start(d, card->due_day);
            }
            break;

        case FSRS_STATE_RELEARNING:
            card->stability  = _s_short(w, card->stability, rating);
            card->difficulty = _d_update(w, card->difficulty, rating);
            if (rating == FSRS_AGAIN) {
                card->step    = 0;
                uint32_t secs = d->_relearn_steps_n ? d->_relearn_steps[0] : 600;
                card->due_ts  = now + (uint64_t)secs;
                card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
            } else if (rating == FSRS_EASY || (uint32_t)(card->step + 1) >= d->_relearn_steps_n) {
                card->state   = FSRS_STATE_REVIEW;
                card->reps++;
                uint32_t days = _next_interval(d, card->stability, true);
                card->due_day = today + days;
                card->due_ts  = fsrs_day_start(d, card->due_day);
            } else {
                card->step++;
                uint32_t secs = d->_relearn_steps[card->step];
                card->due_ts  = now + (uint64_t)secs;
                card->due_day = (uint32_t)fsrs_current_day(d, card->due_ts);
            }
            break;

        default:
            break;
    }

    _enqueue(d, idx);
}

/* ==================== Preview ==================== */

void fsrs_preview(const fsrs_deck *deck, const fsrs_card *card, uint64_t now, uint64_t out[4]) {
    if (!deck || !card || !out) return;

    const fsrs_deck *d = deck;
    const float *w = d->params.w;
    uint64_t today = (uint64_t)fsrs_current_day((fsrs_deck*)d, now);

    uint64_t elapsed_secs = 0;
    if (card->last_review > 0 && now > card->last_review) elapsed_secs = now - card->last_review;
    float r_val = fsrs_retrievability(card->stability, elapsed_secs);

    static const fsrs_rating ratings[4] = { FSRS_AGAIN, FSRS_HARD, FSRS_GOOD, FSRS_EASY };
    uint64_t rng_state_backup = ((fsrs_deck*)d)->rng_state;

    /* 1) Generate raw timestamps */
    for (int ri = 0; ri < 4; ++ri) {
        fsrs_rating rating = ratings[ri];
        uint64_t ts = now;

        switch (card->state) {
            case FSRS_STATE_NEW: {
                float s = _s0(w, rating);
                if (rating == FSRS_AGAIN) {
                    uint32_t secs = d->_learn_steps_n ? d->_learn_steps[0] : 60;
                    ts = now + (uint64_t)secs;
                } else if (d->_learn_steps_n == 0 || rating == FSRS_EASY) {
                    uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                    ts = fsrs_day_start((fsrs_deck*)d, today + days);
                } else {
                    uint32_t step = (rating == FSRS_GOOD) ? 1 : 0;
                    if (step >= d->_learn_steps_n) {
                        uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                        ts = fsrs_day_start((fsrs_deck*)d, today + days);
                    } else {
                        uint32_t secs = d->_learn_steps[step];
                        ts = now + secs;
                    }
                }
                break;
            }

            case FSRS_STATE_LEARNING: {
                float s = _s_short(w, card->stability, rating);
                if (rating == FSRS_AGAIN) {
                    uint32_t secs = d->_learn_steps_n ? d->_learn_steps[0] : 60;
                    ts = now + secs;
                } else if (rating == FSRS_EASY) {
                    uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                    ts = fsrs_day_start((fsrs_deck*)d, today + days);
                } else {
                    uint32_t next_step = card->step + (rating >= FSRS_GOOD ? 1 : 0);
                    if (next_step >= d->_learn_steps_n) {
                        uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                        ts = fsrs_day_start((fsrs_deck*)d, today + days);
                    } else {
                        uint32_t secs = d->_learn_steps[next_step];
                        ts = now + secs;
                    }
                }
                break;
            }

            case FSRS_STATE_RELEARNING: {
                float s = _s_short(w, card->stability, rating);
                if (rating == FSRS_AGAIN) {
                    uint32_t secs = d->_relearn_steps_n ? d->_relearn_steps[0] : 600;
                    ts = now + secs;
                } else if (rating == FSRS_EASY ||
                           (uint32_t)(card->step + 1) >= d->_relearn_steps_n) {
                    uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                    ts = fsrs_day_start((fsrs_deck*)d, today + days);
                } else {
                    uint32_t next_step = card->step + 1;
                    if (next_step >= d->_relearn_steps_n) {
                        uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                        ts = fsrs_day_start((fsrs_deck*)d, today + days);
                    } else {
                        uint32_t secs = d->_relearn_steps[next_step];
                        ts = now + secs;
                    }
                }
                break;
            }

            case FSRS_STATE_REVIEW: {
                if (rating == FSRS_AGAIN) {
                    if (d->_relearn_steps_n > 0) {
                        uint32_t secs = d->_relearn_steps[0];
                        ts = now + secs;
                    } else {
                        float s = _s_forget(w, card->difficulty, card->stability, r_val);
                        uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                        ts = fsrs_day_start((fsrs_deck*)d, today + days);
                    }
                } else {
                    float s = _s_recall(w, card->difficulty, card->stability, r_val, rating);
                    uint32_t days = _next_interval((fsrs_deck*)d, s, false);
                    ts = fsrs_day_start((fsrs_deck*)d, today + days);
                }
                break;
            }

            default:
                ts = now;
        }

        out[ri] = ts;
    }

    /* 2) Convert to deltas and mark day-level intervals */
    uint64_t secs[4];
    bool is_day[4];
    for (int i = 0; i < 4; ++i) {
        uint64_t delta = (out[i] > now) ? (out[i] - now) : 0;
        is_day[i] = (delta >= FSRS_SEC_PER_DAY);
        if (is_day[i]) secs[i] = (delta / FSRS_SEC_PER_DAY) * FSRS_SEC_PER_DAY;
        else secs[i] = delta;
    }

    /* 3) Apply fuzz to day-level intervals */
    for (int i = 1; i < 4; ++i) {
        if (is_day[i]) {
            uint32_t days = (uint32_t)(secs[i] / FSRS_SEC_PER_DAY);
            days = fsrs_fuzz_days((fsrs_deck*)d, days);
            secs[i] = (uint64_t)days * FSRS_SEC_PER_DAY;
        }
    }

    /* 4) Normalize monotonicity */
    for (int i = 1; i < 4; ++i) {
        if (secs[i] <= secs[i-1]) {
            if (is_day[i] && is_day[i-1]) {
                secs[i] = secs[i-1] + FSRS_SEC_PER_DAY;
            } else {
                secs[i] = secs[i-1] + 60;
                if (is_day[i]) {
                    uint64_t days = (secs[i] + FSRS_SEC_PER_DAY - 1) / FSRS_SEC_PER_DAY;
                    secs[i] = days * FSRS_SEC_PER_DAY;
                }
            }
        }
    }

    /* 5) Rebuild out[] timestamps */
    for (int i = 0; i < 4; ++i) {
        out[i] = now + secs[i];
    }

    ((fsrs_deck*)d)->rng_state = rng_state_backup;
}

/* ==================== Statistics ==================== */

void fsrs_compute_stats(const fsrs_deck *d, uint64_t now, fsrs_stats *out) {
    if (!d || !out) return;
    memset(out, 0, sizeof(*out));
    out->total = d->count;
    uint64_t today = fsrs_current_day((fsrs_deck*)d, now);

    float sum_s = 0.0f, sum_d = 0.0f, sum_r = 0.0f;
    uint32_t r_count = 0;

    for (uint32_t i = 0; i < d->count; ++i) {
        const fsrs_card *c = &d->cards[i];
        switch (c->state) {
            case FSRS_STATE_NEW:        out->new_count++;        break;
            case FSRS_STATE_LEARNING:   out->learning_count++;   break;
            case FSRS_STATE_RELEARNING: out->relearning_count++; break;
            case FSRS_STATE_REVIEW:     out->review_count++;     break;
            case FSRS_STATE_SUSPENDED:  out->suspended_count++;  break;
            default: break;
        }
        if (c->flags & FSRS_FLAG_LEECH) out->leech_count++;
        if (c->lapses > 0) out->cards_with_lapses++;
        if (c->due_day <= today) out->due_now++;
        if (c->due_day == today) out->due_today++;

        if (c->stability > 0.0f && c->state != FSRS_STATE_SUSPENDED) {
            sum_s += c->stability;
            sum_d += c->difficulty;
            uint64_t elapsed_secs = 0;
            if (c->last_review > 0 && now > c->last_review) elapsed_secs = now - c->last_review;
            sum_r += fsrs_retrievability(c->stability, elapsed_secs);
            r_count++;
            if (c->stability > 21.0f) out->mature_count++; else out->young_count++;
        }
    }

    if (r_count > 0) {
        out->average_stability  = sum_s / (float)r_count;
        out->average_difficulty = sum_d / (float)r_count;
        out->retention          = sum_r / (float)r_count;
    }
}

void fsrs_format_interval(uint64_t secs, char *buf, size_t len) {
    if (!buf || !len) return;

    uint64_t m  = secs / 60;
    uint64_t h  = secs / 3600;
    uint64_t d  = secs / 86400;
    uint64_t w  = d / 7;
    uint64_t mo = d / 30;
    uint64_t y  = d / 365;

    if (secs < 60)
        snprintf(buf, len, "%lu %s", (unsigned long)secs, secs == 1 ? "segundo" : "segundos");
    else if (m < 60)
        snprintf(buf, len, "%lu %s", (unsigned long)m, m == 1 ? "minuto" : "minutos");
    else if (h < 24)
        snprintf(buf, len, "%lu %s", (unsigned long)h, h == 1 ? "hora" : "horas");
    else if (d < 7)
        snprintf(buf, len, "%lu %s", (unsigned long)d, d == 1 ? "día" : "días");
    else if (d < 30)
        snprintf(buf, len, "%lu %s", (unsigned long)w, w == 1 ? "semana" : "semanas");
    else if (d < 365)
        snprintf(buf, len, "%lu %s", (unsigned long)mo, mo == 1 ? "mes" : "meses");
    else
        snprintf(buf, len, "%lu %s", (unsigned long)y, y == 1 ? "año" : "años");
}

/* ==================== Persistence ==================== */

bool fsrs_save(const fsrs_deck *d, const char *path) {
    if (!d || !path) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint32_t magic   = FSRS_MAGIC;
    uint32_t ver     = FSRS_FILE_VER;
    uint32_t count   = d->count;
    uint64_t cur_day = d->current_day;
    uint32_t nt      = d->new_today;
    uint32_t rt      = d->reviews_today;

    bool ok =
        fwrite(&magic,   4, 1, f) == 1 &&
        fwrite(&ver,     4, 1, f) == 1 &&
        fwrite(&count,   4, 1, f) == 1 &&
        fwrite(&cur_day, 8, 1, f) == 1 &&
        fwrite(&nt,      4, 1, f) == 1 &&
        fwrite(&rt,      4, 1, f) == 1 &&
        (count == 0 || fwrite(d->cards, sizeof(fsrs_card), count, f) == count);

    fclose(f);
    return ok;
}

fsrs_deck* fsrs_load(const char *path, const fsrs_params *params) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    uint32_t magic, ver, count;
    uint64_t cur_day;
    uint32_t nt, rt;
    bool ok =
        fread(&magic,   4, 1, f) == 1 && magic == FSRS_MAGIC &&
        fread(&ver,     4, 1, f) == 1 && ver   == FSRS_FILE_VER &&
        fread(&count,   4, 1, f) == 1 &&
        fread(&cur_day, 8, 1, f) == 1 &&
        fread(&nt,      4, 1, f) == 1 &&
        fread(&rt,      4, 1, f) == 1;

    if (!ok) { fclose(f); return NULL; }

    fsrs_card *tmp = count ? (fsrs_card*)malloc(count * sizeof(fsrs_card)) : NULL;
    if (count && !tmp) { fclose(f); return NULL; }
    if (count && fread(tmp, sizeof(fsrs_card), count, f) != count) { free(tmp); fclose(f); return NULL; }
    fclose(f);

    uint32_t max_id = 0;
    for (uint32_t i = 0; i < count; ++i) if (tmp[i].id > max_id) max_id = tmp[i].id;

    fsrs_deck *d = fsrs_deck_create(max_id + 64, params);
    if (!d) { free(tmp); return NULL; }

    d->current_day  = cur_day;
    d->new_today    = nt;
    d->reviews_today = rt;

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t id = tmp[i].id;
        if (!_bitmap_ensure(d, id) || !_idmap_ensure(d, id)) { fsrs_deck_free(d); free(tmp); return NULL; }
        if (d->count == d->capacity) {
            uint32_t nc = d->capacity ? d->capacity * 2 : 64;
            fsrs_card *na = (fsrs_card*)realloc(d->cards, nc * sizeof(fsrs_card));
            if (!na) { fsrs_deck_free(d); free(tmp); return NULL; }
            d->cards = na; d->capacity = nc;
        }
        uint32_t idx = d->count++;
        d->cards[idx] = tmp[i];
        d->cards[idx].heap_pos_due   = -1;
        d->cards[idx].heap_pos_learn = -1;
        _bitmap_set(d, id, true);
        d->id_to_index[id] = idx;
    }

    free(tmp);
    fsrs_rebuild_queues(d, (uint64_t)time(NULL));
    return d;
}

uint32_t fsrs_deck_count(const fsrs_deck *deck) { return deck ? deck->count : 0; }
const fsrs_card* fsrs_deck_card(const fsrs_deck *deck, uint32_t idx) {
    if (!deck || idx >= deck->count) return NULL;
    return &deck->cards[idx];
}

uint64_t fsrs_now(void) { return (uint64_t)time(NULL); }


void update_srs_config(fsrs_deck *deck, 
    float *w, float desired_retention, uint32_t maximum_interval,
        uint32_t leech_threshold, uint32_t day_offset_secs,
        uint32_t new_cards_per_day, uint32_t reviews_per_day,
        const uint32_t *learning_steps, uint32_t learning_steps_count,
        const uint32_t *relearning_steps, uint32_t relearning_steps_count,
        float new_review_ratio, uint32_t mix_burst_size, bool enable_fuzz,
        fsrs_order_mode order_mode
){

    fsrs_params *params = &deck->params;

    // ignore w for now, as it's not being updated dynamically in this implementation
    if (params->w && w) {
        memcpy(params->w, w, 9 * sizeof(float));
    }

    params->desired_retention = desired_retention;
    params->maximum_interval = maximum_interval;
    params->leech_threshold = leech_threshold;
    params->day_offset_secs = day_offset_secs;
    params->new_cards_per_day = new_cards_per_day;
    params->reviews_per_day = reviews_per_day;
    if (learning_steps && learning_steps_count > 0) {
        params->learning_steps = learning_steps;
        params->learning_steps_count = learning_steps_count;
    }
    if (relearning_steps && relearning_steps_count > 0) {
        params->relearning_steps = relearning_steps;
        params->relearning_steps_count = relearning_steps_count;
    }
    // params->new_review_ratio = new_review_ratio;
    // params->mix_burst_size = mix_burst_size; not in config yet
    params->enable_fuzz = enable_fuzz;
    if (order_mode != FSRS_ORDER_REVIEW_FIRST && order_mode != FSRS_ORDER_NEW_FIRST && order_mode != FSRS_ORDER_MIXED) {
        order_mode = FSRS_ORDER_MIXED; // default to MIXED if invalid
        LOG_DEBUG("Invalid order_mode provided=%d, defaulting to MIXED", order_mode);
    }
    params->order_mode = order_mode;
}