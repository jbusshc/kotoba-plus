#ifndef FSRS_SYNC_H
#define FSRS_SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fsrs.h"

/* Event types */
typedef enum {
    FSRS_EV_ADD     = 0,
    FSRS_EV_REVIEW  = 1,
    FSRS_EV_REMOVE  = 2,
    FSRS_EV_SUSPEND = 3,
    FSRS_EV_UPDATE  = 4,
} fsrs_ev_type;

/* FsrsEvent layout (64 bytes) */
typedef struct {
    uint64_t device_id;
    uint64_t seq;
    uint64_t timestamp;

    uint64_t card_due;
    uint64_t card_last_review;

    uint32_t card_id;

    float    card_stability;
    float    card_difficulty;

    uint16_t card_reps;
    uint16_t card_lapses;

    uint8_t  type;
    uint8_t  rating;
    uint8_t  card_state;
    uint8_t  card_step;
    uint8_t  card_flags;
    uint8_t  _pad[3];
} FsrsEvent;

/* Sync context */
typedef struct {
    uint64_t   device_id;
    uint64_t   next_seq;

    FsrsEvent *events;
    uint32_t   event_count;
    uint32_t   event_cap;
} FsrsSync;

/* Lifecycle */
void fsrs_sync_init(FsrsSync *s, uint64_t device_id);
void fsrs_sync_free(FsrsSync *s);

/* Record local mutations */
FsrsEvent *fsrs_sync_record_add(FsrsSync *s, const fsrs_card *card, uint64_t now);
FsrsEvent *fsrs_sync_record_answer(FsrsSync *s, const fsrs_card *card, fsrs_rating rating, uint64_t now);
FsrsEvent *fsrs_sync_record_remove(FsrsSync *s, uint32_t card_id, uint64_t now);
FsrsEvent *fsrs_sync_record_suspend(FsrsSync *s, const fsrs_card *card, uint64_t now);

/* Merge & rebuild */
void fsrs_sync_merge(FsrsSync *s, const FsrsEvent *remote, uint32_t remote_count);
void fsrs_sync_rebuild(FsrsSync *s, fsrs_deck *deck, uint64_t now);

/* Delta helpers */
uint32_t fsrs_sync_events_since(const FsrsSync *s, uint64_t since_seq, FsrsEvent *out, uint32_t max);
uint64_t fsrs_sync_max_seq_for(const FsrsSync *s, uint64_t device_id);

/* Persistence */
bool fsrs_sync_log_append(const char *path, const FsrsEvent *ev);
bool fsrs_sync_log_load(FsrsSync *s, const char *path);
bool fsrs_sync_snapshot_save(const FsrsSync *s, const fsrs_deck *deck, const char *path);
bool fsrs_sync_snapshot_load(fsrs_deck *deck, const char *path, uint64_t now);
bool fsrs_sync_compact(FsrsSync *s, fsrs_deck *deck, const char *snapshot_path, const char *log_path, uint64_t now);

#ifdef __cplusplus
}
#endif

#endif /* FSRS_SYNC_H */