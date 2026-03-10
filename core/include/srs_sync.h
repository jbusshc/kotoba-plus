#ifndef SRS_SYNC_H
#define SRS_SYNC_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <time.h>

/* =========================
   EVENTO
   ========================= */

typedef enum {
    SRS_EVENT_ADD = 0,
    SRS_EVENT_REVIEW = 1,
    SRS_EVENT_REMOVE = 2
} SrsEventType;

typedef struct {

    uint64_t device_id;
    uint64_t seq;
    uint64_t timestamp;

    uint32_t card_id;
    uint8_t  type;   // SrsEventType
    uint8_t  grade;  // solo válido si type == REVIEW
    uint8_t  padding[2]; // para alineación

} SrsEvent;
/* =========================
   ESTADO DE CARTA (LWW)
   ========================= */

typedef struct {
    uint64_t last_timestamp;
    uint64_t last_device;
    uint64_t last_seq;

    int      interval;
    float    ease;
    time_t   due;
} CardState;

/* =========================
   SYNC PRINCIPAL
   ========================= */

typedef struct {
    uint64_t device_id;
    uint64_t next_seq;

    SrsEvent *events;
    size_t    event_count;
    size_t    event_capacity;

    CardState *cards;
    size_t     card_count;
} SrsSync;

/* =========================
   API PRINCIPAL
   ========================= */

void srs_sync_init(SrsSync *s,
                   uint64_t device_id,
                   size_t card_count);

void srs_sync_free(SrsSync *s);

void srs_sync_add_local_review(SrsSync *s,
                               uint32_t card_id,
                               uint8_t grade,
                               uint64_t timestamp);

/* Nuevo: eventos add/remove locales */
void srs_sync_add_local_add(SrsSync *s,
                            uint32_t card_id,
                            uint64_t timestamp);

void srs_sync_add_local_remove(SrsSync *s,
                               uint32_t card_id,
                               uint64_t timestamp);

void srs_sync_merge_events(SrsSync *s,
                           const SrsEvent *remote,
                           size_t remote_count);

void srs_sync_rebuild(SrsSync *s);

/* =========================
   PERSISTENCIA
   ========================= */

int srs_snapshot_save(SrsSync *s, const char *path);
int srs_snapshot_load(SrsSync *s, const char *path);

int srs_log_save(SrsSync *s, const char *path);
int srs_log_load(SrsSync *s, const char *path);
int srs_log_truncate(const char *path);
int srs_log_append(const char *path, const SrsEvent *e);

int srs_compact(SrsSync *s,
                const char *snapshot_path,
                const char *log_path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif