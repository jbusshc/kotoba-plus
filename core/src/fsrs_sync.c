/*
 * fsrs_sync.c — event-sourced sync layer for the FSRS-5 engine.
 */

#include "fsrs_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* sanity check: FsrsEvent must be 64 bytes */
typedef char _fsrs_event_size_check[(sizeof(FsrsEvent) == 64) ? 1 : -1];

#define SYNC_LOG_MAGIC      0x5352464C
#define SYNC_SNAP_MAGIC     0x53524653
#define SYNC_LOG_VERSION    1u
#define SYNC_SNAP_VERSION   1u

static bool _ev_grow(FsrsSync *s) {
    if (s->event_count < s->event_cap) return true;
    uint32_t nc = s->event_cap ? s->event_cap * 2 : 64;
    FsrsEvent *nb = realloc(s->events, nc * sizeof(FsrsEvent));
    if (!nb) return false;
    s->events = nb;
    s->event_cap = nc;
    return true;
}

/* Copy card fields into an event snapshot */
static void _snap_card(FsrsEvent *ev, const fsrs_card *c) {
    ev->card_state       = c->state;
    ev->card_step        = c->step;
    ev->card_flags       = c->flags;
    ev->card_due         = c->due_ts;         /* timestamp */
    ev->card_last_review = c->last_review;    /* timestamp */
    ev->card_stability   = c->stability;
    ev->card_difficulty  = c->difficulty;
    ev->card_reps        = c->reps;
    ev->card_lapses      = c->lapses;
}

/* create local event in memory */
static FsrsEvent *_new_event(FsrsSync *s, uint32_t card_id, fsrs_ev_type type, uint64_t now) {
    if (!_ev_grow(s)) return NULL;
    FsrsEvent *ev = &s->events[s->event_count++];
    memset(ev, 0, sizeof(*ev));
    ev->device_id = s->device_id;
    ev->seq = s->next_seq++;
    ev->timestamp = now;
    ev->card_id = card_id;
    ev->type = (uint8_t)type;
    return ev;
}

/* LWW comparator */
static bool _lww_wins(const FsrsEvent *a, const FsrsEvent *b) {
    if (a->timestamp != b->timestamp) return a->timestamp > b->timestamp;
    if (a->device_id != b->device_id) return a->device_id > b->device_id;
    return a->seq > b->seq;
}

static int _ev_cmp(const void *pa, const void *pb) {
    const FsrsEvent *a = (const FsrsEvent *)pa;
    const FsrsEvent *b = (const FsrsEvent *)pb;
    if (a->timestamp != b->timestamp)
        return (a->timestamp < b->timestamp) ? -1 : 1;
    if (a->device_id != b->device_id)
        return (a->device_id < b->device_id) ? -1 : 1;
    if (a->seq != b->seq)
        return (a->seq < b->seq) ? -1 : 1;
    return 0;
}

/* lifecycle */
void fsrs_sync_init(FsrsSync *s, uint64_t device_id) {
    memset(s, 0, sizeof(*s));
    s->device_id = device_id;
    s->next_seq = 1;
}

void fsrs_sync_free(FsrsSync *s) {
    free(s->events);
    memset(s, 0, sizeof(*s));
}

/* record local changes (call after core mutation) */
FsrsEvent *fsrs_sync_record_add(FsrsSync *s, const fsrs_card *card, uint64_t now) {
    FsrsEvent *ev = _new_event(s, card->id, FSRS_EV_ADD, now);
    if (!ev) return NULL;
    _snap_card(ev, card);
    return ev;
}

FsrsEvent *fsrs_sync_record_answer(FsrsSync *s, const fsrs_card *card, fsrs_rating rating, uint64_t now) {
    FsrsEvent *ev = _new_event(s, card->id, FSRS_EV_REVIEW, now);
    if (!ev) return NULL;
    ev->rating = (uint8_t)rating;
    _snap_card(ev, card);
    return ev;
}

FsrsEvent *fsrs_sync_record_remove(FsrsSync *s, uint32_t card_id, uint64_t now) {
    FsrsEvent *ev = _new_event(s, card_id, FSRS_EV_REMOVE, now);
    return ev;
}

FsrsEvent *fsrs_sync_record_suspend(FsrsSync *s, const fsrs_card *card, uint64_t now) {
    FsrsEvent *ev = _new_event(s, card->id, FSRS_EV_SUSPEND, now);
    if (!ev) return NULL;
    _snap_card(ev, card);
    return ev;
}

FsrsEvent *fsrs_sync_record_undo(FsrsSync *s, uint32_t card_id, uint64_t now)
{
    /* 1. Marcar el último FSRS_EV_REVIEW de esta carta como cancelado */
    for (int i = (int)s->event_count - 1; i >= 0; i--) {
        FsrsEvent *ev = &s->events[i];
        if (ev->card_id == card_id && ev->type == FSRS_EV_REVIEW) {
            ev->type = FSRS_EV_UNDO;   /* neutralizar en memoria */
            break;
        }
    }

    /* 2. Escribir el evento UNDO al log para sobrevivir un crash */
    FsrsEvent *ev = _new_event(s, card_id, FSRS_EV_UNDO, now);
    return ev;   /* caller hace fsrs_sync_log_append */
}

/* merge remote events into s->events (dedup by device_id+seq) */
void fsrs_sync_merge(FsrsSync *s, const FsrsEvent *remote, uint32_t remote_count) {
    for (uint32_t i = 0; i < remote_count; i++) {
        const FsrsEvent *ev = &remote[i];
        bool found = false;
        for (uint32_t j = 0; j < s->event_count; j++) {
            if (s->events[j].device_id == ev->device_id && s->events[j].seq == ev->seq) { found = true; break; }
        }
        if (found) continue;
        if (!_ev_grow(s)) return;
        s->events[s->event_count++] = *ev;
    }
    /* keep next_seq ahead */
    for (uint32_t i = 0; i < s->event_count; i++) {
        if (s->events[i].device_id == s->device_id && s->events[i].seq >= s->next_seq)
            s->next_seq = s->events[i].seq + 1;
    }
}

/* rebuild: LWW per card -> project state into deck */
void fsrs_sync_rebuild(FsrsSync *s, fsrs_deck *deck, uint64_t now)
{
    if (!s->event_count) {
        fsrs_rebuild_queues(deck, now);
        return;
    }

    /* --- Cancelar REVIEWs anulados por sus UNDOs ------------------------
     * Por cada FSRS_EV_UNDO, buscar hacia atrás el último FSRS_EV_REVIEW
     * de la misma carta y marcarlo también como UNDO para que el LWW
     * winner loop lo ignore. Trabajamos sobre una copia para no mutar
     * el array original antes del qsort. */
    FsrsEvent *evs = malloc(s->event_count * sizeof(FsrsEvent));
    if (!evs) return;
    memcpy(evs, s->events, s->event_count * sizeof(FsrsEvent));

    for (int i = (int)s->event_count - 1; i >= 0; i--) {
        if (evs[i].type != FSRS_EV_UNDO) continue;
        /* buscar el REVIEW más reciente anterior a este UNDO */
        for (int j = i - 1; j >= 0; j--) {
            if (evs[j].card_id == evs[i].card_id &&
                evs[j].type    == FSRS_EV_REVIEW) {
                evs[j].type = FSRS_EV_UNDO;   /* neutralizar */
                break;
            }
        }
    }

    qsort(evs, s->event_count, sizeof(FsrsEvent), _ev_cmp);

    /* --- Unique card ids ----------------------------------------------- */
    uint32_t *ids = NULL;
    uint32_t id_count = 0, id_cap = 0;

    for (uint32_t i = 0; i < s->event_count; ++i) {
        if (evs[i].type == FSRS_EV_UNDO) continue;   /* ignorar */
        uint32_t cid = evs[i].card_id;
        bool found = false;
        for (uint32_t j = 0; j < id_count; j++)
            if (ids[j] == cid) { found = true; break; }
        if (!found) {
            if (id_count == id_cap) {
                id_cap = id_cap ? id_cap * 2 : 32;
                uint32_t *nb = realloc(ids, id_cap * sizeof(uint32_t));
                if (!nb) { free(ids); free(evs); return; }
                ids = nb;
            }
            ids[id_count++] = cid;
        }
    }

    /* --- LWW winner por carta ------------------------------------------ */
    for (uint32_t ci = 0; ci < id_count; ++ci) {
        uint32_t cid = ids[ci];
        const FsrsEvent *winner = NULL;
        for (uint32_t i = 0; i < s->event_count; ++i) {
            const FsrsEvent *ev = &evs[i];
            if (ev->card_id != cid)          continue;
            if (ev->type    == FSRS_EV_UNDO) continue;   /* ignorar */
            if (!winner || _lww_wins(ev, winner)) winner = ev;
        }
        if (!winner) continue;

        if (winner->type == FSRS_EV_REMOVE) {
            if (fsrs_get_card(deck, cid)) fsrs_remove_card(deck, cid);
            continue;
        }

        if (!fsrs_get_card(deck, cid)) {
            fsrs_card *nc = fsrs_add_card(deck, cid, winner->timestamp);
            if (!nc) continue;
        }

        fsrs_card *card = fsrs_get_card(deck, cid);
        if (!card) continue;

        card->state       = winner->card_state;
        card->step        = winner->card_step;
        card->flags       = winner->card_flags;
        card->due_ts      = winner->card_due;
        card->due_day     = (uint32_t)fsrs_current_day(deck, card->due_ts);
        card->last_review = winner->card_last_review;
        card->stability   = winner->card_stability;
        card->difficulty  = winner->card_difficulty;
        card->reps        = winner->card_reps;
        card->lapses      = winner->card_lapses;
    }

    free(ids);
    free(evs);
    fsrs_rebuild_queues(deck, now);
}
/* delta helpers */
uint32_t fsrs_sync_events_since(const FsrsSync *s, uint64_t since_seq, FsrsEvent *out, uint32_t max) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->event_count && n < max; i++) {
        if (s->events[i].device_id == s->device_id && s->events[i].seq > since_seq) out[n++] = s->events[i];
    }
    return n;
}

uint64_t fsrs_sync_max_seq_for(const FsrsSync *s, uint64_t device_id) {
    uint64_t max = 0;
    for (uint32_t i = 0; i < s->event_count; ++i) {
        if (s->events[i].device_id == device_id && s->events[i].seq > max) max = s->events[i].seq;
    }
    return max;
}

/* Log persistence */
#define LOG_HEADER_SIZE  (4 + 4 + 8)

bool fsrs_sync_log_append(const char *path, const FsrsEvent *ev) {
    FILE *f = fopen(path, "ab");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long pos = ftell(f);
    if (pos == 0) {
        uint32_t magic = SYNC_LOG_MAGIC;
        uint32_t version = SYNC_LOG_VERSION;
        bool ok = fwrite(&magic, 4, 1, f) == 1 && fwrite(&version, 4, 1, f) == 1 && fwrite(&ev->device_id, 8, 1, f) == 1;
        if (!ok) { fclose(f); return false; }
    }
    bool ok = fwrite(ev, sizeof(FsrsEvent), 1, f) == 1;
    fclose(f);
    return ok;
}

bool fsrs_sync_log_load(FsrsSync *s, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return true;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size == 0) { fclose(f); return true; }

    uint32_t magic, version;
    uint64_t writer_device;
    if (fread(&magic, 4, 1, f) != 1 || magic != SYNC_LOG_MAGIC ||
        fread(&version, 4, 1, f) != 1 || version != SYNC_LOG_VERSION ||
        fread(&writer_device, 8, 1, f) != 1) {
        fclose(f);
        return false;
    }

    long data_bytes = size - LOG_HEADER_SIZE;
    if (data_bytes < 0 || data_bytes % (long)sizeof(FsrsEvent) != 0) {
        data_bytes -= data_bytes % (long)sizeof(FsrsEvent);
    }

    uint32_t count = (uint32_t)(data_bytes / sizeof(FsrsEvent));
    if (count == 0) { fclose(f); return true; }

    FsrsEvent *tmp = malloc(count * sizeof(FsrsEvent));
    if (!tmp) { fclose(f); return false; }

    uint32_t got = (uint32_t)fread(tmp, sizeof(FsrsEvent), count, f);
    fclose(f);

    fsrs_sync_merge(s, tmp, got);
    free(tmp);
    return true;
}

/* Snapshot record (44 bytes) */
typedef struct {
    uint32_t card_id;
    uint8_t  state;
    uint8_t  step;
    uint8_t  flags;
    uint8_t  _pad;
    uint64_t due;
    uint64_t last_review;
    float    stability;
    float    difficulty;
    uint16_t reps;
    uint16_t lapses;
    uint64_t winning_timestamp;
} _snap_record;

bool fsrs_sync_snapshot_save(const FsrsSync *s, const fsrs_deck *deck, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    uint32_t magic = SYNC_SNAP_MAGIC;
    uint32_t version = SYNC_SNAP_VERSION;
    uint32_t count = fsrs_deck_count(deck);
    bool ok = fwrite(&magic, 4, 1, f) == 1 &&
              fwrite(&version, 4, 1, f) == 1 &&
              fwrite(&s->device_id, 8, 1, f) == 1 &&
              fwrite(&s->next_seq, 8, 1, f) == 1 &&
              fwrite(&count, 4, 1, f) == 1;
    if (!ok) { fclose(f); return false; }

    for (uint32_t i = 0; i < count && ok; ++i) {
        const fsrs_card *c = fsrs_deck_card(deck, i);
        uint64_t win_ts = 0;
        for (uint32_t j = 0; j < s->event_count; ++j)
            if (s->events[j].card_id == c->id && s->events[j].timestamp > win_ts)
                win_ts = s->events[j].timestamp;

        _snap_record rec = {
            .card_id = c->id,
            .state = c->state,
            .step = c->step,
            .flags = c->flags,
            ._pad = 0,
            .due = c->due_ts,            /* store timestamp */
            .last_review = c->last_review,
            .stability = c->stability,
            .difficulty = c->difficulty,
            .reps = c->reps,
            .lapses = c->lapses,
            .winning_timestamp = win_ts
        };
        ok = fwrite(&rec, sizeof(rec), 1, f) == 1;
    }

    fclose(f);
    return ok;
}

bool fsrs_sync_snapshot_load(fsrs_deck *deck, const char *path, uint64_t now) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint32_t magic, version, count;
    uint64_t device_id, next_seq;
    bool ok = fread(&magic, 4, 1, f) == 1 && magic == SYNC_SNAP_MAGIC &&
              fread(&version, 4, 1, f) == 1 && version == SYNC_SNAP_VERSION &&
              fread(&device_id, 8, 1, f) == 1 &&
              fread(&next_seq, 8, 1, f) == 1 &&
              fread(&count, 4, 1, f) == 1;

    if (!ok) { fclose(f); return false; }

    for (uint32_t i = 0; i < count && ok; ++i) {
        _snap_record rec;
        if (fread(&rec, sizeof(rec), 1, f) != 1) { ok = false; break; }

        if (!fsrs_get_card(deck, rec.card_id)) {
            /* rec.due is unix timestamp */
            fsrs_card *nc = fsrs_add_card(deck, rec.card_id, rec.due);
            if (!nc) { ok = false; break; }
        }

        fsrs_card *c = fsrs_get_card(deck, rec.card_id);
        if (!c) { ok = false; break; }

        c->state = rec.state;
        c->step = rec.step;
        c->flags = rec.flags;
        c->due_ts = rec.due;
        c->due_day = (uint32_t)fsrs_current_day(deck, c->due_ts);
        c->last_review = rec.last_review;
        c->stability = rec.stability;
        c->difficulty = rec.difficulty;
        c->reps = rec.reps;
        c->lapses = rec.lapses;
    }

    fclose(f);
    if (ok) fsrs_rebuild_queues(deck, now);
    return ok;
}

bool fsrs_sync_compact(FsrsSync *s, fsrs_deck *deck, const char *snapshot_path, const char *log_path, uint64_t now) {
    fsrs_sync_rebuild(s, deck, now);
    if (!fsrs_sync_snapshot_save(s, deck, snapshot_path)) return false;
    FILE *f = fopen(log_path, "wb");
    if (!f) return false;
    fclose(f);
    free(s->events);
    s->events = NULL;
    s->event_count = 0;
    s->event_cap = 0;
    return true;
}