/*
 * fsrs.c — FSRS-5 Spaced Repetition engine
 *
 * Algorithm reference:
 *   https://github.com/open-spaced-repetition/fsrs4anki/wiki/The-Algorithm
 *
 * Key formulas:
 *   Retrievability:  R(t,S) = (1 + t / (9·S))^(-1)
 *   Stability after recall (good):
 *     S'_r = S · e^(w11) · (11 - D) · S^(-w12) · (e^(w13·(1-R)) - 1) · hardness · ...
 *   Stability after forget (again):
 *     S'_f = w17 · D^(-w15) · ((S+1)^w16 - 1) · e^(w18·(1-R))
 *   Initial stability (first review):
 *     S0 = w[rating-1]   (w0=AGAIN, w1=HARD, w2=GOOD, w3=EASY)
 *   Initial difficulty:
 *     D0 = w4 - e^(w5·(rating-1)) + 1
 *   Difficulty update:
 *     D' = w6·D_target + (1-w6)·(D - w7·(rating-3))
 *     D_target = w4 - e^(w5·(EASY-1)) + 1  (= D0(EASY))
 */

#include "fsrs.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/* ─────────────────────────────────────────────────────────────── *
 * FSRS-5 default weights (from open-spaced-repetition/FSRS-5)
 * ─────────────────────────────────────────────────────────────── */
static const float FSRS5_W[19] = {
    0.4072f,  /* w0:  S0(Again)          */
    1.1829f,  /* w1:  S0(Hard)           */
    3.1262f,  /* w2:  S0(Good)           */
    7.2102f,  /* w3:  S0(Easy)           */
    7.2189f,  /* w4:  D0 intercept       */
    0.5316f,  /* w5:  D0 exponent        */
    1.0651f,  /* w6:  mean reversion     */
    0.0589f,  /* w7:  difficulty change  */
    1.5330f,  /* w8:  recall SInc base   */
   0.14013f,  /* w9:  recall SInc decay  */
    1.0070f,  /* w10: recall hard bonus  */
    1.9395f,  /* w11: recall exponent    */
    0.1100f,  /* w12: stability penalty  */
    0.2900f,  /* w13: retrievability exp */
    2.2700f,  /* w14: forget D exponent  */
    0.2500f,  /* w15: forget S exponent  */
    2.9898f,  /* w16: forget S base      */
    0.5100f,  /* w17: forget R exponent  */
    0.3400f,  /* w18: forget coeff       */
};

/* Default learning steps: 1 min, 10 min */
static uint32_t _default_learn_steps[]   = {60, 600};
static uint32_t _default_relearn_steps[] = {600};

/* ─────────────────────────────────────────────────────────────── *
 * Internal PRNG  (splitmix64, period 2^64)
 * ─────────────────────────────────────────────────────────────── */
static uint64_t _rng_state = 0x9e3779b97f4a7c15ULL;

static uint64_t _rng_next(void) {
    _rng_state += 0x9e3779b97f4a7c15ULL;
    uint64_t z = _rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void fsrs_seed(uint64_t seed) {
    _rng_state = seed ? seed : 0x9e3779b97f4a7c15ULL;
}

/* ─────────────────────────────────────────────────────────────── *
 * Time helpers
 * ─────────────────────────────────────────────────────────────── */
uint64_t fsrs_day_of(const fsrs_deck *d, uint64_t ts) {
    uint32_t off = d->params.day_offset_secs;
    if (ts < off) return 0;
    return (ts - off) / FSRS_SEC_PER_DAY;
}

uint64_t fsrs_day_start(const fsrs_deck *d, uint64_t day) {
    return day * FSRS_SEC_PER_DAY + d->params.day_offset_secs;
}

/* ─────────────────────────────────────────────────────────────── *
 * Default parameters
 * ─────────────────────────────────────────────────────────────── */
fsrs_params fsrs_default_params(void) {
    fsrs_params p;
    memset(&p, 0, sizeof(p));
    memcpy(p.w, FSRS5_W, sizeof(FSRS5_W));
    p.desired_retention   = 0.9f;
    p.maximum_interval    = 36500;
    p.leech_threshold     = 8;
    p.day_offset_secs     = 4 * 3600; /* 04:00 */
    p.new_cards_per_day   = 20;
    p.reviews_per_day     = 200;
    /* steps: caller can override; we use static defaults */
    p.learning_steps      = _default_learn_steps;
    p.learning_steps_count = 2;
    p.relearning_steps    = _default_relearn_steps;
    p.relearning_steps_count = 1;
    return p;
}

/* ─────────────────────────────────────────────────────────────── *
 * Heap helpers  (min-heap on cards[].due, values are indices)
 * ─────────────────────────────────────────────────────────────── */
#define HEAP_PARENT(i) (((i) - 1) >> 1)
#define HEAP_LEFT(i)   (((i) << 1) + 1)
#define HEAP_RIGHT(i)  (((i) << 1) + 2)

/* Compare by due time */
static inline int _heap_less(const fsrs_deck *d, uint32_t a, uint32_t b) {
    return d->cards[a].due < d->cards[b].due;
}

static void _heap_sift_up(fsrs_deck *d, uint32_t *heap, uint32_t size) {
    uint32_t i = size - 1;
    while (i > 0) {
        uint32_t p = HEAP_PARENT(i);
        if (_heap_less(d, heap[i], heap[p])) {
            uint32_t tmp = heap[i]; heap[i] = heap[p]; heap[p] = tmp;
            i = p;
        } else break;
    }
}

static void _heap_sift_down(fsrs_deck *d, uint32_t *heap, uint32_t size, uint32_t i) {
    for (;;) {
        uint32_t smallest = i;
        uint32_t l = HEAP_LEFT(i);
        uint32_t r = HEAP_RIGHT(i);
        if (l < size && _heap_less(d, heap[l], heap[smallest])) smallest = l;
        if (r < size && _heap_less(d, heap[r], heap[smallest])) smallest = r;
        if (smallest == i) break;
        uint32_t tmp = heap[i]; heap[i] = heap[smallest]; heap[smallest] = tmp;
        i = smallest;
    }
}

static bool _heap_push(fsrs_deck *d, uint32_t **heap, uint32_t *size, uint32_t *cap, uint32_t idx) {
    if (*size == *cap) {
        uint32_t nc = (*cap) ? (*cap) * 2 : 8;
        uint32_t *nh = (uint32_t*)realloc(*heap, nc * sizeof(uint32_t));
        if (!nh) return false;
        *heap = nh;
        *cap = nc;
    }
    (*heap)[(*size)++] = idx;
    _heap_sift_up(d, *heap, *size);
    return true;
}

static uint32_t _heap_pop(fsrs_deck *d, uint32_t *heap, uint32_t *size) {
    uint32_t top = heap[0];
    heap[0] = heap[--(*size)];
    _heap_sift_down(d, heap, *size, 0);
    return top;
}

/* Remove arbitrary element by value (linear scan) */
static void _heap_remove(fsrs_deck *d, uint32_t *heap, uint32_t *size, uint32_t idx) {
    for (uint32_t i = 0; i < *size; i++) {
        if (heap[i] == idx) {
            heap[i] = heap[--(*size)];
            if (i < *size) {
                _heap_sift_up(d, heap, i + 1); /* re-use: sift from 0..i */
                _heap_sift_down(d, heap, *size, i);
            }
            return;
        }
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * New-queue helpers  (array, shuffled on demand)
 * ─────────────────────────────────────────────────────────────── */
static bool _newq_push(fsrs_deck *d, uint32_t idx) {
    if (d->new_queue_size == d->new_queue_cap) {
        uint32_t nc = d->new_queue_cap ? d->new_queue_cap * 2 : 8;
        uint32_t *nq = (uint32_t*)realloc(d->new_queue, nc * sizeof(uint32_t));
        if (!nq) return false;
        d->new_queue = nq;
        d->new_queue_cap = nc;
    }
    d->new_queue[d->new_queue_size++] = idx;
    return true;
}

/* Fisher-Yates shuffle of the new queue */
static void _newq_shuffle(fsrs_deck *d) {
    uint32_t n = d->new_queue_size;
    if (n < 2) return;
    for (uint32_t i = n - 1; i > 0; i--) {
        uint32_t j = (uint32_t)(_rng_next() % (uint64_t)(i + 1));
        uint32_t t = d->new_queue[i];
        d->new_queue[i] = d->new_queue[j];
        d->new_queue[j] = t;
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * id_to_index helpers
 * ─────────────────────────────────────────────────────────────── */
#define NO_IDX 0xFFFFFFFFu

static bool _idmap_ensure(fsrs_deck *d, uint32_t id) {
    if (id < d->id_map_size) return true;
    uint32_t nc = d->id_map_size ? d->id_map_size * 2 : 64;
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
    uint32_t nb = d->bitmap_bits ? d->bitmap_bits * 2 : 256;
    while (nb <= id) nb *= 2;
    uint8_t *bm = (uint8_t*)realloc(d->bitmap, nb / 8);
    if (!bm) return false;
    memset(bm + d->bitmap_bits / 8, 0, (nb - d->bitmap_bits) / 8);
    d->bitmap = bm;
    d->bitmap_bits = nb;
    return true;
}

static void _bitmap_set(fsrs_deck *d, uint32_t id, bool v) {
    if (v) d->bitmap[id >> 3] |=  (uint8_t)(1u << (id & 7));
    else   d->bitmap[id >> 3] &= ~(uint8_t)(1u << (id & 7));
}

/* ─────────────────────────────────────────────────────────────── *
 * Deck lifecycle
 * ─────────────────────────────────────────────────────────────── */
bool fsrs_deck_init(fsrs_deck *d, uint32_t id_space, const fsrs_params *params) {
    memset(d, 0, sizeof(*d));
    d->params = params ? *params : fsrs_default_params();

    /* resolve step pointers */
    d->_learn_steps   = d->params.learning_steps
                        ? d->params.learning_steps
                        : _default_learn_steps;
    d->_learn_steps_n = d->params.learning_steps_count
                        ? d->params.learning_steps_count : 2;
    d->_relearn_steps   = d->params.relearning_steps
                          ? d->params.relearning_steps
                          : _default_relearn_steps;
    d->_relearn_steps_n = d->params.relearning_steps_count
                          ? d->params.relearning_steps_count : 1;

    uint32_t bits = id_space ? id_space : 256;
    if (!_bitmap_ensure(d, bits - 1)) goto oom;
    if (!_idmap_ensure(d, bits - 1))  goto oom;
    return true;
oom:
    fsrs_deck_free(d);
    return false;
}

void fsrs_deck_free(fsrs_deck *d) {
    free(d->cards);
    free(d->bitmap);
    free(d->id_to_index);
    free(d->due_heap);
    free(d->learn_heap);
    free(d->new_queue);
    memset(d, 0, sizeof(*d));
}

/* ─────────────────────────────────────────────────────────────── *
 * Card management
 * ─────────────────────────────────────────────────────────────── */
fsrs_card *fsrs_add_card(fsrs_deck *d, uint32_t id, uint64_t now) {
    if (fsrs_has_card(d, id)) return NULL; /* already exists */

    if (!_bitmap_ensure(d, id) || !_idmap_ensure(d, id)) return NULL;

    /* grow card array if needed */
    if (d->count == d->capacity) {
        uint32_t nc = d->capacity ? d->capacity * 2 : 16;
        fsrs_card *na = (fsrs_card*)realloc(d->cards, nc * sizeof(fsrs_card));
        if (!na) return NULL;
        d->cards = na;
        d->capacity = nc;
    }

    uint32_t idx = d->count++;
    fsrs_card *c = &d->cards[idx];
    memset(c, 0, sizeof(*c));
    c->id    = id;
    c->state = FSRS_STATE_NEW;
    c->due   = now; /* new cards are immediately available */

    _bitmap_set(d, id, true);
    d->id_to_index[id] = idx;

    /* push to new queue */
    _newq_push(d, idx);
    return c;
}

bool fsrs_remove_card(fsrs_deck *d, uint32_t id) {
    if (!fsrs_has_card(d, id)) return false;
    uint32_t idx = d->id_to_index[id];
    fsrs_card *c = &d->cards[idx];

    /* remove from whichever queue it's in */
    switch (c->state) {
        case FSRS_STATE_NEW:
            /* find and remove from new_queue (flat array) */
            for (uint32_t i = 0; i < d->new_queue_size; i++) {
                if (d->new_queue[i] == idx) {
                    d->new_queue[i] = d->new_queue[--d->new_queue_size];
                    break;
                }
            }
            break;
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            _heap_remove(d, d->learn_heap, &d->learn_heap_size, idx);
            break;
        case FSRS_STATE_REVIEW:
            _heap_remove(d, d->due_heap, &d->due_heap_size, idx);
            break;
        default: break;
    }

    _bitmap_set(d, id, false);
    d->id_to_index[id] = NO_IDX;

    /* Swap-remove from card array to avoid holes */
    uint32_t last = d->count - 1;
    if (idx != last) {
        d->cards[idx] = d->cards[last];
        uint32_t moved_id = d->cards[idx].id;
        d->id_to_index[moved_id] = idx;
        /* fix references in heaps / new_queue */
        for (uint32_t i = 0; i < d->due_heap_size; i++)
            if (d->due_heap[i] == last) { d->due_heap[i] = idx; break; }
        for (uint32_t i = 0; i < d->learn_heap_size; i++)
            if (d->learn_heap[i] == last) { d->learn_heap[i] = idx; break; }
        for (uint32_t i = 0; i < d->new_queue_size; i++)
            if (d->new_queue[i] == last) { d->new_queue[i] = idx; break; }
    }
    d->count--;
    return true;
}

fsrs_card *fsrs_get_card(fsrs_deck *d, uint32_t id) {
    if (!fsrs_has_card(d, id)) return NULL;
    return &d->cards[d->id_to_index[id]];
}

void fsrs_suspend(fsrs_deck *d, uint32_t id) {
    fsrs_card *c = fsrs_get_card(d, id);
    if (!c || c->state == FSRS_STATE_SUSPENDED) return;
    uint32_t idx = d->id_to_index[id];
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
            _heap_remove(d, d->learn_heap, &d->learn_heap_size, idx);
            break;
        case FSRS_STATE_REVIEW:
            _heap_remove(d, d->due_heap, &d->due_heap_size, idx);
            break;
        default: break;
    }
    c->state = FSRS_STATE_SUSPENDED;
}

void fsrs_unsuspend(fsrs_deck *d, uint32_t id, uint64_t now) {
    fsrs_card *c = fsrs_get_card(d, id);
    if (!c || c->state != FSRS_STATE_SUSPENDED) return;
    uint32_t idx = d->id_to_index[id];
    if (c->reps == 0) {
        c->state = FSRS_STATE_NEW;
        c->due   = now;
        _newq_push(d, idx);
    } else {
        c->state = FSRS_STATE_REVIEW;
        c->due   = now;
        _heap_push(d, &d->due_heap, &d->due_heap_size, &d->due_heap_cap, idx);
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * Rebuild queues  (call after bulk fsrs_load)
 * ─────────────────────────────────────────────────────────────── */
void fsrs_deck_rebuild_queues(fsrs_deck *d, uint64_t now) {
    /* reset */
    d->due_heap_size   = 0;
    d->learn_heap_size = 0;
    d->new_queue_size  = 0;

    for (uint32_t i = 0; i < d->count; i++) {
        fsrs_card *c = &d->cards[i];
        switch (c->state) {
            case FSRS_STATE_NEW:
                _newq_push(d, i);
                break;
            case FSRS_STATE_LEARNING:
            case FSRS_STATE_RELEARNING:
                _heap_push(d, &d->learn_heap, &d->learn_heap_size, &d->learn_heap_cap, i);
                break;
            case FSRS_STATE_REVIEW:
                _heap_push(d, &d->due_heap, &d->due_heap_size, &d->due_heap_cap, i);
                break;
            default: break;
        }
    }
    _newq_shuffle(d);

    /* reset daily counters if day has changed */
    uint64_t today = fsrs_day_of(d, now);
    if (today != d->current_day) {
        d->current_day   = today;
        d->new_today     = 0;
        d->reviews_today = 0;
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * FSRS-5 core algorithm
 * ─────────────────────────────────────────────────────────────── */

/* Clamp difficulty to [1, 10] */
static inline float _clamp_d(float d) {
    return d < 1.0f ? 1.0f : d > 10.0f ? 10.0f : d;
}

/* Initial stability for a given rating */
static inline float _s0(const float w[19], fsrs_rating r) {
    /* r is 1..4, w[0..3] */
    int ri = (int)r - 1;
    if (ri < 0) ri = 0;
    if (ri > 3) ri = 3;
    return fmaxf(w[ri], 0.1f);
}

/* Initial difficulty for a given rating */
static inline float _d0(const float w[19], fsrs_rating r) {
    float d = w[4] - expf(w[5] * (float)((int)r - 1)) + 1.0f;
    return _clamp_d(d);
}

/* Short-term stability increase (same-day re-review, e.g. learning) */
static inline float _s_short(const float w[19], float s, fsrs_rating r) {
    /* S'_short = S · e^(w17 · (r-3 + w18)) */
    float factor = expf(w[17] * ((float)((int)r - 3) + w[18]));
    return fmaxf(s * factor, 0.1f);
}

/* Long-term stability after successful recall */
static float _s_recall(const float w[19], float d, float s, float r_val, fsrs_rating rating) {
    float hard_penalty = (rating == FSRS_HARD) ? w[15] : 1.0f;
    float easy_bonus   = (rating == FSRS_EASY) ? w[16] : 1.0f;
    float new_s = s * (expf(w[8]) * (11.0f - d) * powf(s, -w[9])
                       * (expf(w[10] * (1.0f - r_val)) - 1.0f)
                       * hard_penalty * easy_bonus + 1.0f);
    return fmaxf(new_s, 0.1f);
}

/* Long-term stability after forgetting */
static float _s_forget(const float w[19], float d, float s, float r_val) {
    float new_s = w[11] * powf(d, -w[12]) * (powf(s + 1.0f, w[13]) - 1.0f)
                  * expf(w[14] * (1.0f - r_val));
    return fmaxf(new_s, 0.1f);
}

/* Difficulty update after a review */
static float _d_update(const float w[19], float d, fsrs_rating r) {
    /* Mean reversion target = D0(EASY) */
    float d_target = _d0(w, FSRS_EASY);
    float delta = -w[6] * (float)((int)r - 3);
    float new_d  = w[7] * d_target + (1.0f - w[7]) * (d + delta);
    return _clamp_d(new_d);
}

/* Next interval given target retention and stability */
static uint32_t _next_interval(const fsrs_params *p, float s) {
    if (s <= 0.0f) return 1;
    float i = (float)(9.0 * (double)s * ((1.0 / (double)p->desired_retention) - 1.0));
    uint32_t days = (uint32_t)(i + 0.5f);
    if (days < 1) days = 1;
    if (days > p->maximum_interval) days = p->maximum_interval;
    return days;
}

/* Snap a review due date to the start of the logical day N days from now.
 * This mirrors Anki's behavior: interval is in whole days, and the card
 * becomes available at the day boundary (e.g. 04:00), not at the exact
 * time of answering. */
static inline uint64_t _due_snap(const fsrs_deck *d, uint64_t now, uint32_t days) {
    uint64_t today = fsrs_day_of(d, now);
    return fsrs_day_start(d, today + (uint64_t)days);
}

/* ─────────────────────────────────────────────────────────────── *
 * Internal: place card in the right queue after state change
 * ─────────────────────────────────────────────────────────────── */
static void _enqueue(fsrs_deck *d, uint32_t idx) {
    fsrs_card *c = &d->cards[idx];
    switch (c->state) {
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            _heap_push(d, &d->learn_heap, &d->learn_heap_size, &d->learn_heap_cap, idx);
            break;
        case FSRS_STATE_REVIEW:
            _heap_push(d, &d->due_heap, &d->due_heap_size, &d->due_heap_cap, idx);
            break;
        default: break;
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * fsrs_next_card — peek at the next due card
 * ─────────────────────────────────────────────────────────────── */

/* Reset daily counters if logical day changed */
static void _maybe_new_day(fsrs_deck *d, uint64_t now) {
    uint64_t today = fsrs_day_of(d, now);
    if (today != d->current_day) {
        d->current_day   = today;
        d->new_today     = 0;
        d->reviews_today = 0;
    }
}

fsrs_card *fsrs_next_card(fsrs_deck *d, uint64_t now) {
    _maybe_new_day(d, now);

    /* End of the current logical day — reviews due within today are fair game */
    uint64_t day_end = fsrs_day_start(d, d->current_day + 1);

    /* ── Priority 1: learning/relearning cards whose timer has expired ── */
    if (d->learn_heap_size > 0) {
        uint32_t idx = d->learn_heap[0];
        if (d->cards[idx].due <= now) return &d->cards[idx];
    }

    /* ── Priority 2: reviews due today (within daily limit) ── */
    uint32_t rlimit  = d->params.reviews_per_day;
    bool review_ok   = (rlimit == 0) || (d->reviews_today < rlimit);
    if (review_ok && d->due_heap_size > 0) {
        uint32_t idx = d->due_heap[0];
        if (d->cards[idx].due <= day_end) return &d->cards[idx];
    }

    /* ── Priority 3: new cards (within daily limit) ── */
    uint32_t nlimit = d->params.new_cards_per_day;
    bool new_ok     = (nlimit == 0) || (d->new_today < nlimit);
    if (new_ok && d->new_queue_size > 0) {
        return &d->cards[d->new_queue[0]];
    }

    /* ── Priority 4: learning cards whose timer has NOT expired yet ──
     *
     * Anki behaviour: if nothing else is left in the session, show the
     * next learning card immediately rather than making the user wait.
     * This only kicks in when there are zero reviews/new left to show.
     */
    if (d->learn_heap_size > 0) {
        return &d->cards[d->learn_heap[0]];
    }

    return NULL;
}

/* Seconds until the next card becomes available (or 0 if one is ready now).
 * Useful for showing a "next card in Xs" indicator in the UI. */
uint64_t fsrs_wait_seconds(fsrs_deck *d, uint64_t now) {
    _maybe_new_day(d, now);
    uint64_t day_end = fsrs_day_start(d, d->current_day + 1);

    /* A card is already ready */
    if (fsrs_next_card(d, now)) return 0;

    /* Find the earliest future due timestamp across all queues */
    uint64_t earliest = UINT64_MAX;

    if (d->learn_heap_size > 0) {
        uint64_t t = d->cards[d->learn_heap[0]].due;
        if (t < earliest) earliest = t;
    }

    uint32_t rlimit = d->params.reviews_per_day;
    bool review_ok  = (rlimit == 0) || (d->reviews_today < rlimit);
    if (review_ok && d->due_heap_size > 0) {
        uint64_t t = d->cards[d->due_heap[0]].due;
        if (t <= day_end && t < earliest) earliest = t;
    }

    if (earliest == UINT64_MAX || earliest <= now) return 0;
    return earliest - now;
}

/* ─────────────────────────────────────────────────────────────── *
 * fsrs_answer — core scheduling
 * ─────────────────────────────────────────────────────────────── */
void fsrs_answer(fsrs_deck *d, fsrs_card *card, fsrs_rating rating, uint64_t now) {
    _maybe_new_day(d, now);

    const float *w = d->params.w;
    uint32_t idx   = (uint32_t)(card - d->cards);

    /* elapsed time since last review */
    uint64_t elapsed = (card->last_review && now > card->last_review)
                       ? now - card->last_review : 0;
    float r_val = fsrs_retrievability(card->stability, elapsed);

    /* ── Remove from current queue ── */
    switch (card->state) {
        case FSRS_STATE_NEW:
            for (uint32_t i = 0; i < d->new_queue_size; i++) {
                if (d->new_queue[i] == idx) {
                    d->new_queue[i] = d->new_queue[--d->new_queue_size];
                    break;
                }
            }
            d->new_today++;
            break;
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING:
            _heap_remove(d, d->learn_heap, &d->learn_heap_size, idx);
            break;
        case FSRS_STATE_REVIEW:
            _heap_remove(d, d->due_heap, &d->due_heap_size, idx);
            d->reviews_today++;
            break;
        default: break;
    }

    card->last_review = now;

    /* ── STATE MACHINE ── */
    switch (card->state) {

    /* ── NEW → LEARNING ── */
    case FSRS_STATE_NEW:
        card->stability  = _s0(w, rating);
        card->difficulty = _d0(w, rating);
        card->reps       = 0;
        card->lapses     = 0;
        card->step       = 0;

        if (rating == FSRS_AGAIN) {
            card->state = FSRS_STATE_LEARNING;
            card->step  = 0;
            card->due   = now + d->_learn_steps[0];
        } else if (d->_learn_steps_n == 0 || rating == FSRS_EASY) {
            /* Graduate immediately */
            card->state = FSRS_STATE_REVIEW;
            uint32_t days = _next_interval(&d->params, card->stability);
            card->due   = _due_snap(d, now, days);
            card->reps  = 1;
        } else {
            /* Advance through learning steps */
            card->state = FSRS_STATE_LEARNING;
            uint32_t step = (rating == FSRS_HARD) ? 0 :
                            (rating == FSRS_GOOD) ? 0 : 1; /* GOOD → step 1 if exists */
            if (step >= d->_learn_steps_n) {
                /* Graduate */
                card->state = FSRS_STATE_REVIEW;
                uint32_t days = _next_interval(&d->params, card->stability);
                card->due   = _due_snap(d, now, days);
                card->reps  = 1;
                break;
            }
            card->step = (uint8_t)step;
            card->due  = now + d->_learn_steps[step];
        }
        break;

    /* ── LEARNING ── */
    case FSRS_STATE_LEARNING: {
        /* Update stability short-term */
        card->stability = _s_short(w, card->stability, rating);
        card->difficulty = _d_update(w, card->difficulty, rating);

        if (rating == FSRS_AGAIN) {
            card->step = 0;
            card->due  = now + d->_learn_steps[0];
        } else if (rating == FSRS_EASY) {
            /* Graduate */
            card->state = FSRS_STATE_REVIEW;
            card->reps++;
            uint32_t days = _next_interval(&d->params, card->stability);
            card->due   = _due_snap(d, now, days);
        } else {
            /* HARD or GOOD: advance step */
            uint32_t next_step = card->step + (rating >= FSRS_GOOD ? 1 : 0);
            if (next_step >= d->_learn_steps_n) {
                /* Graduate */
                card->state = FSRS_STATE_REVIEW;
                card->reps++;
                uint32_t days = _next_interval(&d->params, card->stability);
                card->due   = _due_snap(d, now, days);
            } else {
                card->step = (uint8_t)next_step;
                card->due  = now + d->_learn_steps[next_step];
            }
        }
        break;
    } /* FSRS_STATE_LEARNING */

    /* ── REVIEW ── */
    case FSRS_STATE_REVIEW:
        if (rating == FSRS_AGAIN) {
            /* Lapse */
            card->lapses++;
            card->stability  = _s_forget(w, card->difficulty, card->stability, r_val);
            card->difficulty = _d_update(w, card->difficulty, rating);

            /* Leech detection */
            if (d->params.leech_threshold > 0 &&
                card->lapses >= d->params.leech_threshold) {
                card->flags |= FSRS_FLAG_LEECH;
            }

            if (d->_relearn_steps_n > 0) {
                card->state = FSRS_STATE_RELEARNING;
                card->step  = 0;
                card->due   = now + d->_relearn_steps[0];
            } else {
                /* No relearning steps: reschedule directly */
                uint32_t days = _next_interval(&d->params, card->stability);
                card->due   = _due_snap(d, now, days);
            }
        } else {
            /* Recall: update stability and difficulty */
            card->stability  = _s_recall(w, card->difficulty, card->stability, r_val, rating);
            card->difficulty = _d_update(w, card->difficulty, rating);
            card->reps++;
            uint32_t days = _next_interval(&d->params, card->stability);
            card->due = _due_snap(d, now, days);
        }
        break;

    /* ── RELEARNING ── */
    case FSRS_STATE_RELEARNING:
        card->stability  = _s_short(w, card->stability, rating);
        card->difficulty = _d_update(w, card->difficulty, rating);

        if (rating == FSRS_AGAIN) {
            card->step = 0;
            card->due  = now + d->_relearn_steps[0];
        } else if (rating == FSRS_EASY ||
                   (uint32_t)(card->step + 1) >= d->_relearn_steps_n) {
            /* Graduate back to review */
            card->state = FSRS_STATE_REVIEW;
            card->reps++;
            uint32_t days = _next_interval(&d->params, card->stability);
            card->due   = _due_snap(d, now, days);
        } else {
            card->step++;
            card->due = now + d->_relearn_steps[card->step];
        }
        break;

    default: break;
    }

    /* Re-enqueue */
    _enqueue(d, idx);
}

/* ─────────────────────────────────────────────────────────────── *
 * fsrs_preview
 * ─────────────────────────────────────────────────────────────── */
void fsrs_preview(const fsrs_deck *d, const fsrs_card *card,
                  uint64_t now, uint64_t out[4]) {
    /* We simulate the answer for each rating without touching the real card */
    const float *w  = d->params.w;
    uint64_t elapsed = (card->last_review && now > card->last_review)
                       ? now - card->last_review : 0;
    float r_val = fsrs_retrievability(card->stability, elapsed);

    static const fsrs_rating ratings[4] = {
        FSRS_AGAIN, FSRS_HARD, FSRS_GOOD, FSRS_EASY
    };

    for (int ri = 0; ri < 4; ri++) {
        fsrs_rating rating = ratings[ri];
        uint64_t due = now; /* fallback */

        switch (card->state) {
        case FSRS_STATE_NEW: {
            float s = _s0(w, rating);
            if (rating == FSRS_AGAIN) {
                due = now + (d->_learn_steps_n ? d->_learn_steps[0] : 60);
            } else if (d->_learn_steps_n == 0 || rating == FSRS_EASY) {
                due = _due_snap(d, now, _next_interval(&d->params, s));
            } else {
                due = now + d->_learn_steps[0];
            }
            break;
        }
        case FSRS_STATE_LEARNING:
        case FSRS_STATE_RELEARNING: {
            float s = _s_short(w, card->stability, rating);
            const uint32_t *steps = (card->state == FSRS_STATE_LEARNING)
                                    ? d->_learn_steps : d->_relearn_steps;
            uint32_t n = (card->state == FSRS_STATE_LEARNING)
                         ? d->_learn_steps_n : d->_relearn_steps_n;
            if (rating == FSRS_AGAIN) {
                due = now + (n ? steps[0] : 60);
            } else if (rating == FSRS_EASY || (uint32_t)(card->step + 1) >= n) {
                due = _due_snap(d, now, _next_interval(&d->params, s));
            } else {
                due = now + steps[card->step + 1];
            }
            break;
        }
        case FSRS_STATE_REVIEW: {
            float s;
            if (rating == FSRS_AGAIN) {
                s = _s_forget(w, card->difficulty, card->stability, r_val);
                if (d->_relearn_steps_n > 0) {
                    due = now + d->_relearn_steps[0];
                } else {
                    due = _due_snap(d, now, _next_interval(&d->params, s));
                }
            } else {
                s = _s_recall(w, card->difficulty, card->stability, r_val, rating);
                due = _due_snap(d, now, _next_interval(&d->params, s));
            }
            break;
        }
        default:
            due = now;
        }
        out[ri] = due;
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * Statistics
 * ─────────────────────────────────────────────────────────────── */
void fsrs_compute_stats(const fsrs_deck *d, uint64_t now, fsrs_stats *out) {
    memset(out, 0, sizeof(*out));
    out->total = d->count;

    uint64_t day_end = fsrs_day_start((fsrs_deck*)d,
                       fsrs_day_of((fsrs_deck*)d, now) + 1);

    float sum_s = 0.0f, sum_d = 0.0f, sum_r = 0.0f;
    uint32_t r_count = 0;

    for (uint32_t i = 0; i < d->count; i++) {
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
        if (c->due <= now) out->due_now++;
        if (c->due <= day_end) out->due_today++;

        if (c->stability > 0.0f && c->state != FSRS_STATE_SUSPENDED) {
            sum_s += c->stability;
            sum_d += c->difficulty;
            uint64_t elapsed = (c->last_review && now > c->last_review)
                               ? now - c->last_review : 0;
            sum_r += fsrs_retrievability(c->stability, elapsed);
            r_count++;
        }
    }

    if (r_count > 0) {
        out->average_stability   = sum_s / (float)r_count;
        out->average_difficulty  = sum_d / (float)r_count;
        out->retention           = sum_r / (float)r_count;
    }
}

/* ─────────────────────────────────────────────────────────────── *
 * Persistence  (compact binary format)
 *
 * File layout:
 *   [4] magic  "FSRS"
 *   [4] version (uint32, little-endian)
 *   [4] count
 *   [8] current_day
 *   [4] new_today
 *   [4] reviews_today
 *   [count × sizeof(fsrs_card)] cards
 * ─────────────────────────────────────────────────────────────── */
#define FSRS_MAGIC   0x53525346u  /* "FSRS" */
#define FSRS_FILE_VER 1u

bool fsrs_save(const fsrs_deck *d, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint32_t magic = FSRS_MAGIC, ver = FSRS_FILE_VER;
    uint32_t count = d->count;
    uint64_t cur_day = d->current_day;
    uint32_t nt = d->new_today, rt = d->reviews_today;

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

bool fsrs_load(fsrs_deck *d, const char *path, const fsrs_params *params) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

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

    if (!ok) { fclose(f); return false; }

    /* find max id to size structures */
    uint32_t max_id = 0;
    fsrs_card *tmp = count ? (fsrs_card*)malloc(count * sizeof(fsrs_card)) : NULL;
    if (count && !tmp) { fclose(f); return false; }
    if (count && fread(tmp, sizeof(fsrs_card), count, f) != count) {
        free(tmp); fclose(f); return false;
    }
    fclose(f);

    for (uint32_t i = 0; i < count; i++)
        if (tmp[i].id > max_id) max_id = tmp[i].id;

    /* initialise deck */
    if (!fsrs_deck_init(d, max_id + 64, params)) { free(tmp); return false; }
    d->current_day   = cur_day;
    d->new_today     = nt;
    d->reviews_today = rt;

    /* bulk-insert cards */
    for (uint32_t i = 0; i < count; i++) {
        uint32_t id = tmp[i].id;
        if (!_bitmap_ensure(d, id) || !_idmap_ensure(d, id)) { free(tmp); return false; }
        if (d->count == d->capacity) {
            uint32_t nc = d->capacity ? d->capacity * 2 : 16;
            fsrs_card *na = (fsrs_card*)realloc(d->cards, nc * sizeof(fsrs_card));
            if (!na) { free(tmp); return false; }
            d->cards = na; d->capacity = nc;
        }
        uint32_t idx = d->count++;
        d->cards[idx] = tmp[i];
        _bitmap_set(d, id, true);
        d->id_to_index[id] = idx;
    }
    free(tmp);

    fsrs_deck_rebuild_queues(d, (uint64_t)time(NULL));
    return true;
}

/* ─────────────────────────────────────────────────────────────── *
 * Utilities
 * ─────────────────────────────────────────────────────────────── */
void fsrs_format_interval(uint64_t secs, char *buf, size_t len) {
    if (!buf || !len) return;
    uint64_t m = secs / 60;
    uint64_t h = secs / 3600;
    uint64_t d = secs / 86400;
    uint64_t w = d / 7;
    uint64_t mo= d / 30;
    uint64_t y = d / 365;

    if      (secs < 60)  snprintf(buf, len, "%lus",   (unsigned long)secs);
    else if (m    < 60)  snprintf(buf, len, "%lum",   (unsigned long)m);
    else if (h    < 24)  snprintf(buf, len, "%luh",   (unsigned long)h);
    else if (d    < 7)   snprintf(buf, len, "%lud",   (unsigned long)d);
    else if (w    < 4)   snprintf(buf, len, "%luw",   (unsigned long)w);
    else if (mo   < 12)  snprintf(buf, len, "%lumo",  (unsigned long)mo);
    else                 snprintf(buf, len, "%luy",   (unsigned long)y);
}