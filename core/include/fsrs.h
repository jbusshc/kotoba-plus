/*
 * fsrs.h — FSRS-5 Spaced Repetition engine
 *
 * Now supports:
 *  - due_day (days since epoch) -> scheduling logic (FSRS)
 *  - due_ts  (unix seconds)     -> cooldown / ordering inside a day
 *  - per-card heap indices for O(log n) removals
 */

#ifndef FSRS_H
#define FSRS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSRS_SEC_PER_DAY        86400ULL
#define FSRS_MIN_STABILITY      0.01f
#define FSRS_MAX_STABILITY      36500.0f
#define FSRS_MIN_DIFFICULTY     1.0f
#define FSRS_MAX_DIFFICULTY     10.0f
#define FSRS_DEFAULT_OFFSET     14400ULL

typedef enum {
    FSRS_AGAIN = 1,
    FSRS_HARD  = 2,
    FSRS_GOOD  = 3,
    FSRS_EASY  = 4
} fsrs_rating;

typedef enum {
    FSRS_STATE_NEW = 0,
    FSRS_STATE_LEARNING,
    FSRS_STATE_REVIEW,
    FSRS_STATE_RELEARNING,
    FSRS_STATE_SUSPENDED
} fsrs_state;

typedef enum {
    FSRS_QUEUE_NEW,
    FSRS_QUEUE_LEARN,
    FSRS_QUEUE_REVIEW,
    FSRS_QUEUE_DAY_LEARN,
    FSRS_QUEUE_NONE
} fsrs_queue_type;

typedef enum {
    FSRS_FLAG_NONE    = 0,
    FSRS_FLAG_LEECH   = 1 << 0,
    FSRS_FLAG_MARKED  = 1 << 1,
    FSRS_FLAG_DUE     = 1 << 2,
    FSRS_FLAG_SNOOZED = 1 << 3
} fsrs_flags;

typedef struct {
    float w[19];
    float desired_retention;
    uint32_t maximum_interval;
    uint32_t leech_threshold;
    uint32_t day_offset_secs;

    uint32_t new_cards_per_day;
    uint32_t reviews_per_day;

    /* learning/relearning steps are in seconds */
    const uint32_t *learning_steps;
    uint32_t learning_steps_count;

    const uint32_t *relearning_steps;
    uint32_t relearning_steps_count;

    float new_review_ratio;
    uint32_t mix_burst_size;
    bool enable_fuzz;
} fsrs_params;

/* Card structure: due_day is FSRS day index, due_ts is unix seconds for ordering/cooldown.
 * heap_pos_due / heap_pos_learn store the position in each heap (-1 = not present). */
typedef struct __attribute__((packed)) {
    uint32_t id;

    uint32_t due_day;       /* day index used by FSRS algorithm */
    uint64_t due_ts;        /* unix timestamp used for ordering / cooldown */
    uint64_t last_review;   /* last review unix seconds (0 = never) */

    float stability;
    float difficulty;

    uint16_t reps;
    uint16_t lapses;
    uint32_t total_answers;   /* every fsrs_answer() call, all states */

    int32_t heap_pos_due;   /* position in due_heap (or -1) */
    int32_t heap_pos_learn; /* position in learn_heap (or -1) */

    uint8_t state;
    uint8_t step;
    uint8_t flags;
    uint8_t prev_state;
} fsrs_card;

typedef struct {
    uint64_t session_start;
    uint32_t cards_done;
    uint32_t session_limit;
    uint32_t new_done;
    uint32_t review_done;
    uint32_t learn_done;
} fsrs_session;

typedef struct {
    uint32_t total;
    uint32_t new_count;
    uint32_t learning_count;
    uint32_t relearning_count;
    uint32_t review_count;
    uint32_t suspended_count;
    uint32_t leech_count;
    uint32_t cards_with_lapses;
    uint32_t due_now;
    uint32_t due_today;
    float average_stability;
    float average_difficulty;
    float retention;
    uint32_t mature_count;
    uint32_t young_count;
} fsrs_stats;

typedef struct fsrs_deck fsrs_deck;

/* API */
fsrs_params fsrs_default_params(void);
fsrs_deck* fsrs_deck_create(uint32_t id_space, const fsrs_params *params);
void fsrs_deck_free(fsrs_deck *deck);
void fsrs_seed(uint64_t seed);

fsrs_card* fsrs_add_card(fsrs_deck *deck, uint32_t id, uint64_t now);
bool fsrs_remove_card(fsrs_deck *deck, uint32_t id);
fsrs_card* fsrs_get_card(fsrs_deck *deck, uint32_t id);
void fsrs_suspend(fsrs_deck *deck, uint32_t id);
void fsrs_unsuspend(fsrs_deck *deck, uint32_t id, uint64_t now);
void fsrs_mark(fsrs_deck *deck, uint32_t id, bool marked);
void fsrs_snooze(fsrs_deck *deck, uint32_t id, uint32_t days, uint64_t now);

fsrs_card* fsrs_next_card(fsrs_deck *deck, uint64_t now);
fsrs_card* fsrs_next_card_ex(fsrs_deck *deck, uint64_t now, fsrs_queue_type *out_queue);
fsrs_card* fsrs_next_for_session(fsrs_deck *deck, fsrs_session *session, uint64_t now);
void fsrs_session_start(fsrs_session *session, uint64_t now, uint32_t limit);
uint64_t fsrs_wait_seconds(fsrs_deck *deck, uint64_t now);
void fsrs_rebuild_queues(fsrs_deck *deck, uint64_t now);

void fsrs_answer(fsrs_deck *deck, fsrs_card *card, fsrs_rating rating, uint64_t now);
void fsrs_preview(const fsrs_deck *deck, const fsrs_card *card, uint64_t now, uint64_t out[4]);

static inline float fsrs_retrievability(float stability, uint64_t elapsed_secs) {
    if (stability <= 0.0f) return 0.0f;
    float t = (float)elapsed_secs / FSRS_SEC_PER_DAY;
    return 1.0f / (1.0f + t / (9.0f * stability));
}

uint64_t fsrs_current_day(fsrs_deck *deck, uint64_t now);
uint64_t fsrs_day_start(const fsrs_deck *deck, uint64_t day);

void fsrs_compute_stats(const fsrs_deck *deck, uint64_t now, fsrs_stats *out);
void fsrs_format_interval(uint64_t secs, char *buf, size_t len);

bool fsrs_save(const fsrs_deck *deck, const char *path);
fsrs_deck* fsrs_load(const char *path, const fsrs_params *params);

uint32_t fsrs_deck_count(const fsrs_deck *deck);
const fsrs_card* fsrs_deck_card(const fsrs_deck *deck, uint32_t idx);
uint64_t fsrs_now(void);

#ifdef __cplusplus
}
#endif

#endif /* FSRS_H */