#include "srs_sync.h"

#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 *  Helpers
 * ───────────────────────────────────────────────────────────── */

static int event_cmp(const void *a, const void *b)
{
    const srs_event *x = a;
    const srs_event *y = b;

    if (x->timestamp != y->timestamp)
        return (x->timestamp < y->timestamp) ? -1 : 1;

    if (x->device_id != y->device_id)
        return (x->device_id < y->device_id) ? -1 : 1;

    if (x->seq != y->seq)
        return (x->seq < y->seq) ? -1 : 1;

    return 0;
}

static bool write_event(FILE *fp, const srs_event *e)
{
    return fwrite(e, sizeof(*e), 1, fp) == 1;
}

/* ─────────────────────────────────────────────────────────────
 *  Snapshot
 * ───────────────────────────────────────────────────────────── */

static bool save_snapshot(FILE *fp,
                          const srs_profile *p,
                          uint32_t event_count)
{
    rewind(fp);

    fwrite(&event_count, sizeof(event_count), 1, fp);
    fwrite(p->items, sizeof(srs_item), p->count, fp);
    fwrite(&p->count, sizeof(p->count), 1, fp);

    fflush(fp);
    return true;
}

static bool load_snapshot(FILE *fp,
                          srs_profile *p,
                          uint32_t dict_size,
                          uint32_t *out_event_count)
{
    rewind(fp);

    if (fread(out_event_count, sizeof(*out_event_count), 1, fp) != 1)
        return false;

    uint32_t count;
    if (fread(&count, sizeof(count), 1, fp) != 1)
        return false;

    srs_init(p, dict_size);

    for (uint32_t i = 0; i < count; ++i) {
        fread(&p->items[i], sizeof(srs_item), 1, fp);
    }
    p->count = count;

    return true;
}

/* ─────────────────────────────────────────────────────────────
 *  Open / Close
 * ───────────────────────────────────────────────────────────── */

bool srs_sync_open(srs_sync *s,
                   const char *events_path,
                   const char *snapshot_path,
                   uint32_t device_id,
                   uint32_t dict_size)
{
    memset(s, 0, sizeof(*s));
    s->device_id = device_id;

    s->events_fp = fopen(events_path, "ab+");
    if (!s->events_fp) return false;

    s->snapshot_fp = fopen(snapshot_path, "rb+");
    if (!s->snapshot_fp)
        s->snapshot_fp = fopen(snapshot_path, "wb+");

    fseek(s->events_fp, 0, SEEK_END);
    long sz = ftell(s->events_fp);
    s->event_count = (uint32_t)(sz / sizeof(srs_event));

    s->next_seq = 1;
    return true;
}

void srs_sync_close(srs_sync *s)
{
    if (s->events_fp) fclose(s->events_fp);
    if (s->snapshot_fp) fclose(s->snapshot_fp);
}

/* ─────────────────────────────────────────────────────────────
 *  Append events
 * ───────────────────────────────────────────────────────────── */

static bool append_event(srs_sync *s, const srs_event *e)
{
    if (!write_event(s->events_fp, e))
        return false;

    fflush(s->events_fp);
    s->event_count++;

    return true;
}

bool srs_sync_add(srs_sync *s, srs_profile *p,
                  uint32_t entry_id, uint64_t now)
{
    if (!srs_add(p, entry_id, now)) return false;

    srs_event e = {
        .timestamp = now,
        .entry_id  = entry_id,
        .device_id = s->device_id,
        .seq       = s->next_seq++,
        .type      = SRS_EVT_ADD
    };

    return append_event(s, &e);
}

bool srs_sync_remove(srs_sync *s, srs_profile *p,
                     uint32_t entry_id, uint64_t now)
{
    if (!srs_remove(p, entry_id)) return false;

    srs_event e = {
        .timestamp = now,
        .entry_id  = entry_id,
        .device_id = s->device_id,
        .seq       = s->next_seq++,
        .type      = SRS_EVT_REMOVE
    };

    return append_event(s, &e);
}

bool srs_sync_review(srs_sync *s, srs_profile *p,
                     uint32_t entry_id,
                     srs_quality q,
                     uint64_t now)
{
    for (uint32_t i = 0; i < p->count; ++i) {
        if (p->items[i].entry_id == entry_id) {
            srs_answer(&p->items[i], q, now);
            break;
        }
    }

    srs_event e = {
        .timestamp = now,
        .entry_id  = entry_id,
        .device_id = s->device_id,
        .seq       = s->next_seq++,
        .type      = SRS_EVT_REVIEW,
        .quality   = q
    };

    return append_event(s, &e);
}

/* ─────────────────────────────────────────────────────────────
 *  Rebuild (snapshot + replay)
 * ───────────────────────────────────────────────────────────── */

bool srs_sync_rebuild(srs_sync *s, srs_profile *p, uint32_t dict_size)
{
    uint32_t snap_events = 0;

    if (!load_snapshot(s->snapshot_fp, p, dict_size, &snap_events)) {
        srs_init(p, dict_size);
        snap_events = 0;
    }

    fseek(s->events_fp,
          snap_events * sizeof(srs_event),
          SEEK_SET);

    srs_event e;
    while (fread(&e, sizeof(e), 1, s->events_fp) == 1) {
        switch (e.type) {
        case SRS_EVT_ADD:
            srs_add(p, e.entry_id, e.timestamp);
            break;
        case SRS_EVT_REMOVE:
            srs_remove(p, e.entry_id);
            break;
        case SRS_EVT_REVIEW:
            for (uint32_t i = 0; i < p->count; ++i)
                if (p->items[i].entry_id == e.entry_id)
                    srs_answer(&p->items[i],
                               (srs_quality)e.quality,
                               e.timestamp);
            break;
        }
    }

    if (s->event_count - snap_events >= SRS_SNAPSHOT_INTERVAL) {
        save_snapshot(s->snapshot_fp, p, s->event_count);
    }

    return true;
}

/* ─────────────────────────────────────────────────────────────
 *  Merge remote events
 * ───────────────────────────────────────────────────────────── */

bool srs_sync_merge_events(srs_sync *s,
                           const srs_event *events,
                           uint32_t count)
{
    fseek(s->events_fp, 0, SEEK_END);

    for (uint32_t i = 0; i < count; ++i)
        write_event(s->events_fp, &events[i]);

    fflush(s->events_fp);
    s->event_count += count;
    return true;
}
