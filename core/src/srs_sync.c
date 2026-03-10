#include "srs_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* =========================================================
   UTILIDADES INTERNAS
   ========================================================= */

static int srs_event_compare(const void *a, const void *b)
{
    const SrsEvent *ea = (const SrsEvent*)a;
    const SrsEvent *eb = (const SrsEvent*)b;

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

static void srs_recalculate(CardState *card,
                            uint8_t grade,
                            uint64_t timestamp)
{
    if (card->ease < 1.3f)
        card->ease = 2.5f;

    if (grade < 2) {
        card->interval = 1;
    } else {
        if (card->interval == 0)
            card->interval = 1;
        else
            card->interval = (int)((float)card->interval * card->ease + 0.5f);
    }

    card->ease += 0.1f * (grade - 2);

    if (card->ease < 1.3f)
        card->ease = 1.3f;

    card->due = timestamp + (uint64_t)card->interval * 86400ULL;
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

    srs_recalculate(card, ev->grade, ev->timestamp);
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
    if (card_id >= s->card_count)
        return;
    srs_ensure_capacity(s);

    SrsEvent *ev = &s->events[s->event_count++];

    ev->device_id = s->device_id;
    ev->seq = s->next_seq++;
    ev->timestamp = timestamp;
    ev->card_id = card_id;

    ev->type = SRS_EVENT_REVIEW;
    ev->grade = grade;
}

/* Nuevas funciones: ADD / REMOVE locales.
   Usan valores especiales de 'grade' para distinguirse:
     250 -> ADD
     251 -> REMOVE
   (no cambiamos la estructura SrsEvent para mantener compatibilidad)
*/
void srs_sync_add_local_add(SrsSync *s,
                            uint32_t card_id,
                            uint64_t timestamp)
{
    srs_ensure_capacity(s);

    SrsEvent *ev = &s->events[s->event_count++];

    ev->device_id = s->device_id;
    ev->seq = s->next_seq++;
    ev->timestamp = timestamp;
    ev->card_id = card_id;

    ev->type = SRS_EVENT_ADD;
    ev->grade = 0;
}

void srs_sync_add_local_remove(SrsSync *s,
                               uint32_t card_id,
                               uint64_t timestamp)
{
    srs_ensure_capacity(s);

    SrsEvent *ev = &s->events[s->event_count++];

    ev->device_id = s->device_id;
    ev->seq = s->next_seq++;
    ev->timestamp = timestamp;
    ev->card_id = card_id;

    ev->type = SRS_EVENT_REMOVE;
    ev->grade = 0;
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
    if (!s) return;

    /* Reset estado de cartas */
    memset(s->cards, 0, sizeof(CardState) * s->card_count);

    for (size_t i = 0; i < s->card_count; i++) {
        s->cards[i].ease = 2.5f;
        s->cards[i].interval = 0;
    }

    /* Ordenar eventos para replay determinístico */
    qsort(s->events,
          s->event_count,
          sizeof(SrsEvent),
          srs_event_compare);

    /* Reaplicar eventos */
    for (size_t i = 0; i < s->event_count; i++) {

        SrsEvent *ev = &s->events[i];

        if (ev->card_id >= s->card_count)
            continue;

        CardState *card = &s->cards[ev->card_id];

        /* REMOVE */
        if (ev->type == SRS_EVENT_REMOVE) {

            memset(card, 0, sizeof(CardState));
            card->ease = 2.5f;

            card->last_timestamp = ev->timestamp;
            card->last_device    = ev->device_id;
            card->last_seq       = ev->seq;

            continue;
        }

        /* ADD */
        if (ev->type == SRS_EVENT_ADD) {

            if (ev->timestamp >= card->last_timestamp) {
                card->last_timestamp = ev->timestamp;
                card->last_device    = ev->device_id;
                card->last_seq       = ev->seq;

                card->interval = 0;
                card->ease = 2.5f;
                card->due = 0;
            }

            continue;
        }

        /* REVIEW */
        if (ev->type == SRS_EVENT_REVIEW && ev->grade <= 5) {

            srs_apply_event_lww(card, ev);
        }
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

    if (s->event_count > 0) {
        if (fwrite(s->events, sizeof(SrsEvent), s->event_count, f) != s->event_count) {
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

int srs_log_load(SrsSync *s, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size == 0) { fclose(f); return 1; }

    /* Caso 1: archivo es un stream puro de SrsEvent (nuevo formato esperado) */
    if (size % sizeof(SrsEvent) == 0) {
        size_t count = size / sizeof(SrsEvent);
        SrsEvent *tmp = realloc(s->events, count * sizeof(SrsEvent));
        if (!tmp) { fclose(f); return 0; }
        s->events = tmp;
        s->event_capacity = count;
        s->event_count = count;

        if (fread(s->events, sizeof(SrsEvent), count, f) != count) { fclose(f); return 0; }

        fclose(f);
        s->next_seq = srs_compute_next_seq(s);
        return 1;
    }

    /* Caso 2: formato antiguo con encabezado count (compatibilidad) */
    if (size > (long)sizeof(size_t)) {
        size_t count = 0;
        if (fread(&count, sizeof(size_t), 1, f) != 1) { fclose(f); return 0; }
        if ((long)(size - sizeof(size_t)) != (long)count * (long)sizeof(SrsEvent)) { fclose(f); return 0; }

        SrsEvent *tmp = realloc(s->events, count * sizeof(SrsEvent));
        if (!tmp) { fclose(f); return 0; }
        s->events = tmp;
        s->event_capacity = count;
        s->event_count = count;

        if (fread(s->events, sizeof(SrsEvent), count, f) != count) { fclose(f); return 0; }

        fclose(f);
        s->next_seq = srs_compute_next_seq(s);
        return 1;
    }

    fclose(f);
    return 0;
}

int srs_log_truncate(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
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


int srs_log_append(const char *path, const SrsEvent *e)
{
    FILE *f = fopen(path, "ab");
    if (!f) return 0;

    if (fwrite(e, sizeof(SrsEvent), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}