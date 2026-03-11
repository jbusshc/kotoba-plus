#include "srs.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

/* ─────────────────────────────────────────────
    Defaults & Config
    ───────────────────────────────────────────── */

static const uint64_t default_learning_steps[] = { 10*60, 50*60, SRS_DAY };
#define DEFAULT_LEARNING_STEPS_COUNT (sizeof(default_learning_steps)/sizeof(default_learning_steps[0]))
#define INTERVAL_FUZZ_PCT 0.05f
#define LEECH_THRESHOLD 8
static int srs_random_seeded = 0;

/* ─────────────────────────────────────────────
    Random & Bitmap helpers
    ───────────────────────────────────────────── */

static inline void bitmap_set(uint8_t *bm, uint32_t id) { bm[id >> 3] |= (1u << (id & 7)); }
static inline void bitmap_clear(uint8_t *bm, uint32_t id) { bm[id >> 3] &= ~(1u << (id & 7)); }

void srs_seed_random(uint32_t seed) { srand(seed); srs_random_seeded = 1; }

static inline uint32_t srs_rand_u32(void) {
     if (!srs_random_seeded) { srand((unsigned)time(NULL)); srs_random_seeded = 1; }
     return ((uint32_t)rand() << 16) ^ (uint32_t)rand();
}

static inline double srs_random_double(double low, double high) {
     uint32_t r = srs_rand_u32();
     double unit = (double)r / (double)UINT32_MAX;
     return low + unit * (high - low);
}

static inline float fsrs_fuzz(void) { return 0.97f + ((rand() % 7) / 100.0f); }

static inline float clampf(float v, float min, float max) {
     if (v < min) return min;
     if (v > max) return max;
     return v;
}

/* ─────────────────────────────────────────────
    Heap ops
    ───────────────────────────────────────────── */

static inline bool heap_less_items(const srs_profile *p, uint32_t ia, uint32_t ib)
{
     return p->items[ia].due < p->items[ib].due;
}

static void heap_sift_up(srs_profile *p, uint32_t *heap, uint32_t heap_size, uint32_t idx_pos)
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

static void heap_sift_down(srs_profile *p, uint32_t *heap, uint32_t heap_size, uint32_t idx_pos)
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

/* ─────────────────────────────────────────────
    New_stack shuffle
    ───────────────────────────────────────────── */

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

/* ─────────────────────────────────────────────
    Promote day learning
    ───────────────────────────────────────────── */

static void srs_promote_day_learning(srs_profile *p, uint64_t now)
{
     uint64_t today = srs_today(now);
     while (p->day_learning_heap_size > 0) {
          uint32_t idx = p->day_learning_heap[0];
          srs_item *it = &p->items[idx];
          if (srs_today(it->due) > today)
                break;
          heap_pop(p, p->day_learning_heap, &p->day_learning_heap_size);
          heap_push_learning(p, idx);
     }
}

/* ─────────────────────────────────────────────
    Lifecycle: init/free
    ───────────────────────────────────────────── */

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

     p->daily_new_limit = 20;
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

/* ─────────────────────────────────────────────
    Scheduling helpers
    ───────────────────────────────────────────── */

static void srs_update_day_if_needed(srs_profile *p, uint64_t now)
{
     uint64_t today = srs_today(now);
     if (p->current_logical_day != today) {
          p->current_logical_day = today;
          p->new_served_today = 0;
          p->review_served_today = 0;
     }
}

void srs_heapify(srs_profile *p)
{
     if (!p) return;

     uint32_t bitmap_size = (p->dict_size + 7) / 8;
     if (p->bitmap && bitmap_size > 0)
          memset(p->bitmap, 0, bitmap_size);

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

     shuffle_new_stack(p);
}

/* ─────────────────────────────────────────────
    Peek/pop due
    ───────────────────────────────────────────── */

srs_item *srs_peek_due(srs_profile *p, uint64_t now)
{
     if (!p) return NULL;

     srs_update_day_if_needed(p, now);
     srs_promote_day_learning(p, now);

     if (p->learning_heap_size > 0) {
          uint32_t idx = p->learning_heap[0];
          if (p->items[idx].due <= now)
                return &p->items[idx];
     }

     if (p->review_heap_size > 0) {
          uint32_t idx = p->review_heap[0];
          if (p->items[idx].due <= now) {
                if (p->daily_review_limit == 0 || p->review_served_today < p->daily_review_limit)
                     return &p->items[idx];
          }
     }

     if (p->new_stack_size > 0) {
          if (p->daily_new_limit == 0 || p->new_served_today < p->daily_new_limit) {
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

     if (p->learning_heap_size > 0) {
          uint32_t idx = p->learning_heap[0];
          if (p->items[idx].due <= now) {
                uint32_t item_index = heap_pop(p, p->learning_heap, &p->learning_heap_size);
                out->index = item_index;
                out->item = &p->items[item_index];
                if (p->daily_review_limit != 0)
                     p->review_served_today++;
                return true;
          }
     }

     if (p->review_heap_size > 0) {
          uint32_t idx = p->review_heap[0];
          if (p->items[idx].due <= now) {
                if (p->daily_review_limit != 0 && p->review_served_today >= p->daily_review_limit)
                     return false;
                uint32_t item_index = heap_pop(p, p->review_heap, &p->review_heap_size);
                out->index = item_index;
                out->item = &p->items[item_index];
                if (p->daily_review_limit != 0)
                     p->review_served_today++;
                return true;
          }
     }

     if (p->new_stack_size > 0) {
          if (p->daily_new_limit != 0 && p->new_served_today >= p->daily_new_limit)
                return false;
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

/* ─────────────────────────────────────────────
    Requeue
    ───────────────────────────────────────────── */

void srs_requeue(srs_profile *p, uint32_t index)
{
     if (!p || index >= p->count) return;

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
     } else if (it->state == SRS_STATE_LEARNING || it->state == SRS_STATE_RELEARNING) {
          heap_push_learning(p, index);
     } else if (it->state == SRS_STATE_REVIEW) {
          heap_push_review(p, index);
     }
}

/* ─────────────────────────────────────────────
    SM-2
    ───────────────────────────────────────────── */

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

void srs_answer_sm2(srs_profile *p, srs_item *it, srs_quality q, uint64_t now)
{
     if (!it) return;
     if (it->state == SRS_STATE_SUSPENDED)
          return;

     if (it->state == SRS_STATE_NEW) {
          it->state = SRS_STATE_LEARNING;
          it->step = 0;
          it->due = now + p->learning_steps[0];
          return;
     }

     if (it->state == SRS_STATE_LEARNING || it->state == SRS_STATE_RELEARNING) {
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

          it->state = SRS_STATE_REVIEW;
          it->reps = 1;
          it->interval = (q == SRS_EASY) ? 4 : 1;

          uint64_t today = srs_today(now);
          uint64_t due_day = today + it->interval;
          it->due = srs_day_to_unix(due_day);
          return;
     }

     if (it->state == SRS_STATE_REVIEW) {
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
          } else {
                float factor;

                if (q == SRS_HARD) {
                     factor = it->ease * 0.8f;
                     it->ease -= 0.15f;
                } else if (q == SRS_GOOD) {
                     factor = it->ease;
                } else {
                     factor = it->ease * 1.3f;
                     it->ease += 0.15f;
                }

                uint32_t next = (uint32_t)(it->interval * factor);
                if (next < 1) next = 1;
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

/* ─────────────────────────────────────────────
    FSRS 
    ───────────────────────────────────────────── */

void srs_answer_fsrs(srs_profile *p, srs_item *it, srs_quality q, uint64_t now)
{
    if (!it) return;

    // Normaliza la calidad a 0..1
    float quality = clampf((float)q / 5.0f, 0.0f, 1.0f);

    // Parámetros FSRS
    const float retention_target = 0.9f;   // objetivo de retención
    const float difficulty_min = 0.3f;
    const float difficulty_max = 1.8f;
    const uint64_t day_seconds = 24*3600;

    // --- NEW ---
    if (it->state == SRS_STATE_NEW) {
        it->state = SRS_STATE_LEARNING;
        it->step = 0;
        it->stability = 1.0f;          // valor inicial arbitrario
        it->difficulty = 0.6f;         // dificultad inicial
        it->due = now + p->learning_steps[0]; 
        it->retention = 1.0f;
        return;
    }

    // --- LEARNING / RELEARNING ---
    if (it->state == SRS_STATE_LEARNING || it->state == SRS_STATE_RELEARNING) {
        if (q < SRS_HARD) {
            // error: reinicia pasos
            it->step = 0;
            it->due = now + p->learning_steps[0];
            return;
        }

        it->step++;
        if (it->step < p->learning_steps_count) {
            it->due = now + p->learning_steps[it->step];
            return;
        }

        // pasa a review
        it->state = SRS_STATE_REVIEW;
        it->reps = 1;
        it->due = now + day_seconds;
        return;
    }

    // --- REVIEW ---
    if (it->state == SRS_STATE_REVIEW) {
        // Ajuste de dificultad según calidad
        float diff_delta = 0.1f * (retention_target - quality);
        it->difficulty = clampf(it->difficulty + diff_delta, difficulty_min, difficulty_max);

        // Nueva estabilidad según FSRS: S' = S * exp(log(R_target) / D)
        float old_stability = it->stability;
        it->stability = old_stability * expf(logf(retention_target) / it->difficulty);

        // Aplica un pequeño fuzz para variabilidad natural
        it->stability *= fsrs_fuzz();

        if (quality < retention_target) {
            // mala respuesta → relearning
            it->state = SRS_STATE_RELEARNING;
            it->lapses++;
            it->due = now + p->learning_steps[0]; // primer paso de relearning
        } else {
            // buena respuesta → aumenta reps y programa próxima revisión
            it->reps++;
            it->due = now + (uint64_t)(it->stability * day_seconds);
        }

        it->retention = quality;
        return;
    }

    // --- RELEARNING ---
    if (it->state == SRS_STATE_RELEARNING) {
        if (q >= SRS_GOOD) {
            // respuesta suficientemente buena → vuelve a review
            it->state = SRS_STATE_REVIEW;
            it->due = now + (uint64_t)(it->stability * day_seconds);
        } else {
            // respuesta mala → repite paso inicial
            it->due = now + p->learning_steps[0];
        }
    }
}

/* ─────────────────────────────────────────────
    FSRS / SM2 selector
    ───────────────────────────────────────────── */

void srs_answer(srs_profile *p, srs_item *it, srs_quality q, uint64_t now)
{
     if (!it) return;
     switch (p->mode) {
          case SRS_MODE_SM2:
                srs_answer_sm2(p, it, q, now);
                break;
          case SRS_MODE_FSRS:
                srs_answer_fsrs(p, it, q, now);
                break;
     }
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
          p->items = realloc(p->items, p->capacity * sizeof(srs_item));
          p->review_heap = realloc(p->review_heap, p->capacity * sizeof(uint32_t));
          p->learning_heap = realloc(p->learning_heap, p->capacity * sizeof(uint32_t));
          p->new_stack = realloc(p->new_stack, p->capacity * sizeof(uint32_t));
          p->day_learning_heap = realloc(p->day_learning_heap, p->capacity * sizeof(uint32_t));
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

     p->new_stack[p->new_stack_size++] = idx;
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
    if (!p || entry_id >= p->dict_size || !srs_contains(p, entry_id))
        return false;

    for (uint32_t i = 0; i < p->count; ++i) {
        if (p->items[i].entry_id == entry_id) {
            bitmap_clear(p->bitmap, entry_id);

            // Reemplazamos la carta con la última
            uint32_t last_idx = p->count - 1;
            if (i != last_idx) {
                p->items[i] = p->items[last_idx];

                // Actualizamos heaps
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
    if (fwrite(&p->count, sizeof(p->count), 1, fp) != 1) goto fail;

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

    uint32_t dict_size = 0, count = 0;

    if (fread(&dict_size, sizeof(dict_size), 1, fp) != 1) goto fail;
    if (dict_size != expected_dict_size) goto fail;
    if (fread(&count, sizeof(count), 1, fp) != 1) goto fail;

    srs_free(p);
    if (!srs_init(p, dict_size)) goto fail;

    if (count > 0) {
        if (count > p->capacity) {
            p->items = realloc(p->items, sizeof(srs_item) * count);
            p->review_heap = realloc(p->review_heap, sizeof(uint32_t) * count);
            p->learning_heap = realloc(p->learning_heap, sizeof(uint32_t) * count);
            p->new_stack = realloc(p->new_stack, sizeof(uint32_t) * count);
            p->day_learning_heap = realloc(p->day_learning_heap, sizeof(uint32_t) * count);
            p->capacity = count;
        }

        if (fread(p->items, sizeof(srs_item), count, fp) != count) goto fail;
        p->count = count;
    }

    uint32_t bitmap_size = (dict_size + 7) / 8;
    if (bitmap_size > 0) {
        if (fread(p->bitmap, 1, bitmap_size, fp) != bitmap_size)
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
    Config
    ───────────────────────────────────────────── */

void srs_set_limits(srs_profile *p, uint32_t daily_new_limit, uint32_t daily_review_limit)
{
     if (!p) return;
     p->daily_new_limit = daily_new_limit;
     p->daily_review_limit = daily_review_limit;
}

bool srs_set_learning_steps(srs_profile *p, const uint64_t *steps, uint32_t count)
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
