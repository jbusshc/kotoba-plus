#include "srs_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================
   UTILIDADES INTERNAS
   ========================================================= */

static int srs_event_compare(const void *a, const void *b)
{
    const SrsEvent *ea = a;
    const SrsEvent *eb = b;

    if (ea->timestamp != eb->timestamp)
        return (ea->timestamp < eb->timestamp) ? -1 : 1;

    if (ea->device_id != eb->device_id)
        return (ea->device_id < eb->device_id) ? -1 : 1;

    if (ea->seq != eb->seq)
        return (ea->seq < eb->seq) ? -1 : 1;

    return 0;
}

static int srs_event_exists(const SrsSync *s,
                            uint64_t device_id,
                            uint64_t seq)
{
    for (size_t i = 0; i < s->event_count; i++) {
        if (s->events[i].device_id == device_id &&
            s->events[i].seq == seq)
            return 1;
    }
    return 0;
}

static uint64_t srs_compute_next_seq(const SrsSync *s)
{
    uint64_t max_seq = 0;

    for (size_t i = 0; i < s->event_count; i++) {
        if (s->events[i].device_id == s->device_id) {
            if (s->events[i].seq > max_seq)
                max_seq = s->events[i].seq;
        }
    }

    return max_seq + 1;
}

static void srs_ensure_capacity(SrsSync *s)
{
    if (s->event_count < s->event_capacity)
        return;

    size_t new_cap = (s->event_capacity == 0)
        ? 128
        : s->event_capacity * 2;

    SrsEvent *new_events =
        realloc(s->events, new_cap * sizeof(SrsEvent));

    if (!new_events)
        abort();

    s->events = new_events;
    s->event_capacity = new_cap;
}

/* =========================================================
   SRS LWW
   ========================================================= */

static void srs_recalculate(CardState *card, uint8_t grade)
{
    if (card->ease < 1.3f)
        card->ease = 2.5f;

    
    if (grade < 2) {
        card->interval = 1;
    } else {
        if (card->interval == 0)
            card->interval = 1;
        else
            card->interval = (int)(card->interval * card->ease);
    }

    card->ease += 0.1f * (grade - 2);
    if (card->ease < 1.3f)
        card->ease = 1.3f;

    card->due = time(NULL) + card->interval * 86400;
}

static void srs_apply_event_lww(CardState *card,
                                const SrsEvent *ev)
{
    if (ev->timestamp < card->last_timestamp)
        return;

    if (ev->timestamp == card->last_timestamp) {
        if (ev->device_id < card->last_device)
            return;

        if (ev->device_id == card->last_device &&
            ev->seq <= card->last_seq)
            return;
    }

    card->last_timestamp = ev->timestamp;
    card->last_device    = ev->device_id;
    card->last_seq       = ev->seq;

    srs_recalculate(card, ev->grade);
}

/* =========================================================
   API PRINCIPAL
   ========================================================= */

void srs_sync_init(SrsSync *s,
                   uint64_t device_id,
                   size_t card_count)
{
    memset(s, 0, sizeof(*s));

    s->device_id = device_id;
    s->card_count = card_count;

    s->cards = calloc(card_count, sizeof(CardState));
    if (!s->cards)
        abort();
}

void srs_sync_free(SrsSync *s)
{
    free(s->events);
    free(s->cards);
}

void srs_sync_add_local_review(SrsSync *s,
                               uint32_t card_id,
                               uint8_t grade,
                               uint64_t timestamp)
{
    srs_ensure_capacity(s);

    SrsEvent *ev = &s->events[s->event_count++];

    ev->device_id = s->device_id;
    ev->seq       = s->next_seq++;
    ev->timestamp = timestamp;
    ev->card_id   = card_id;
    ev->grade     = grade;
}

void srs_sync_merge_events(SrsSync *s,
                           const SrsEvent *remote,
                           size_t remote_count)
{
    for (size_t i = 0; i < remote_count; i++) {

        const SrsEvent *ev = &remote[i];

        if (srs_event_exists(s,
                             ev->device_id,
                             ev->seq))
            continue;

        srs_ensure_capacity(s);
        s->events[s->event_count++] = *ev;
    }

    s->next_seq = srs_compute_next_seq(s);
}

void srs_sync_rebuild(SrsSync *s)
{
    memset(s->cards, 0,
        sizeof(CardState) * s->card_count);

    for (size_t i = 0; i < s->card_count; i++) {
        s->cards[i].ease = 2.5f;   // valor base SM-2 clásico
        s->cards[i].interval = 0;
    }


    qsort(s->events,
          s->event_count,
          sizeof(SrsEvent),
          srs_event_compare);

    for (size_t i = 0; i < s->event_count; i++) {

        SrsEvent *ev = &s->events[i];

        if (ev->card_id >= s->card_count)
            continue;

        srs_apply_event_lww(
            &s->cards[ev->card_id],
            ev);
    }

    s->next_seq = srs_compute_next_seq(s);
}

/* =========================================================
   SNAPSHOT
   ========================================================= */

int srs_snapshot_save(SrsSync *s, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    uint32_t version = 1;

    fwrite(&version, sizeof(uint32_t), 1, f);
    fwrite(&s->device_id, sizeof(uint64_t), 1, f);
    fwrite(&s->next_seq, sizeof(uint64_t), 1, f);
    fwrite(&s->card_count, sizeof(size_t), 1, f);

    for (size_t i = 0; i < s->card_count; i++) {
        CardState *c = &s->cards[i];

        fwrite(&c->last_timestamp, sizeof(uint64_t), 1, f);
        fwrite(&c->last_device, sizeof(uint64_t), 1, f);
        fwrite(&c->last_seq, sizeof(uint64_t), 1, f);

        fwrite(&c->interval, sizeof(int), 1, f);
        fwrite(&c->ease, sizeof(float), 1, f);
        fwrite(&c->due, sizeof(time_t), 1, f);
    }

    fclose(f);
    return 1;
}
int srs_snapshot_load(SrsSync *s, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    uint32_t version;

    fread(&version, sizeof(uint32_t), 1, f);
    if (version != 1) {
        fclose(f);
        return 0;
    }

    fread(&s->device_id, sizeof(uint64_t), 1, f);
    fread(&s->next_seq, sizeof(uint64_t), 1, f);
    fread(&s->card_count, sizeof(size_t), 1, f);

    CardState *tmp = realloc(s->cards,
                             sizeof(CardState) * s->card_count);
    if (!tmp) {
        fclose(f);
        return 0;
    }

    s->cards = tmp;

    for (size_t i = 0; i < s->card_count; i++) {
        CardState *c = &s->cards[i];

        fread(&c->last_timestamp, sizeof(uint64_t), 1, f);
        fread(&c->last_device, sizeof(uint64_t), 1, f);
        fread(&c->last_seq, sizeof(uint64_t), 1, f);

        fread(&c->interval, sizeof(int), 1, f);
        fread(&c->ease, sizeof(float), 1, f);
        fread(&c->due, sizeof(time_t), 1, f);
    }

    fclose(f);
    return 1;
}


/* =========================================================
   LOG
   ========================================================= */

int srs_log_save(SrsSync *s, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    fwrite(&s->event_count, sizeof(size_t), 1, f);
    fwrite(s->events,
           sizeof(SrsEvent),
           s->event_count,
           f);

    fclose(f);
    return 1;
}

int srs_log_load(SrsSync *s, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    fread(&s->event_count, sizeof(size_t), 1, f);

    s->event_capacity = s->event_count;
    SrsEvent *tmp = realloc(s->events,
                            sizeof(SrsEvent) * s->event_capacity);

    if (!tmp) {
        fclose(f);
        return 0;
    }

    s->events = tmp;


    fread(s->events,
          sizeof(SrsEvent),
          s->event_count,
          f);

    fclose(f);

    s->next_seq = srs_compute_next_seq(s);

    return 1;
}

int srs_log_truncate(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    size_t zero = 0;
    fwrite(&zero, sizeof(size_t), 1, f);

    fclose(f);
    return 1;
}

/* =========================================================
   COMPACTION
   ========================================================= */

int srs_compact(SrsSync *s,
                const char *snapshot_path,
                const char *log_path)
{
    srs_sync_rebuild(s);

    if (!srs_snapshot_save(s, snapshot_path))
        return 0;

    if (!srs_log_truncate(log_path))
        return 0;

    free(s->events);
    s->events = NULL;
    s->event_count = 0;
    s->event_capacity = 0;

    return 1;
}
