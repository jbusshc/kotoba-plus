#ifndef SRS_SYNC_H
#define SRS_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "srs.h"

#include "kotoba.h"

/* ─────────────────────────────────────────────────────────────
 *  Configuration
 * ───────────────────────────────────────────────────────────── */

#define SRS_SNAPSHOT_INTERVAL 1000   /* events */

/* ─────────────────────────────────────────────────────────────
 *  Event types
 * ───────────────────────────────────────────────────────────── */

typedef enum {
    SRS_EVT_ADD    = 1,
    SRS_EVT_REMOVE = 2,
    SRS_EVT_REVIEW = 3
} srs_event_type;

/* ─────────────────────────────────────────────────────────────
 *  Event
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    uint64_t timestamp;     /* unix seconds */
    uint32_t entry_id;

    uint32_t device_id;     /* unique per device */
    uint32_t seq;           /* monotonic per device */

    uint8_t  type;          /* srs_event_type */
    uint8_t  quality;       /* review only */

    uint8_t  reserved[2];
} srs_event;

/* ─────────────────────────────────────────────────────────────
 *  Snapshot
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    uint64_t last_event_time;
    uint32_t last_event_count;

    srs_profile profile;
} srs_snapshot;

/* ─────────────────────────────────────────────────────────────
 *  Sync handle
 * ───────────────────────────────────────────────────────────── */

typedef struct {
    FILE *events_fp;
    FILE *snapshot_fp;

    uint32_t device_id;
    uint32_t next_seq;

    uint32_t event_count;
} srs_sync;

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/* init / shutdown */
KOTOBA_API bool srs_sync_open(srs_sync *s,
                   const char *events_path,
                   const char *snapshot_path,
                   uint32_t device_id,
                   uint32_t dict_size);

KOTOBA_API void srs_sync_close(srs_sync *s);

/* append events */
KOTOBA_API bool srs_sync_add(srs_sync *s, srs_profile *p,
                  uint32_t entry_id, uint64_t now);

KOTOBA_API bool srs_sync_remove(srs_sync *s, srs_profile *p,
                     uint32_t entry_id, uint64_t now);

KOTOBA_API bool srs_sync_review(srs_sync *s, srs_profile *p,
                     uint32_t entry_id,
                     srs_quality q,
                     uint64_t now);

/* replay & merge */
KOTOBA_API bool srs_sync_rebuild(srs_sync *s, srs_profile *p, uint32_t dict_size);

/* import remote events */
KOTOBA_API bool srs_sync_merge_events(srs_sync *s,
                           const srs_event *events,
                           uint32_t count);

#endif /* SRS_SYNC_H */
