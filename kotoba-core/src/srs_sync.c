#include "srs_sync.h"

#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 *  Internal helpers
 * ───────────────────────────────────────────────────────────── */

static bool write_event(FILE *fp, const srs_event *e)
{
    return fwrite(e, sizeof(*e), 1, fp) == 1;
}

/* ------------------------------------------------------------- */
/* Rebuild bitmap + heap after snapshot load                     */
/* ------------------------------------------------------------- */

static void rebuild_runtime_structures(srs_profile *p)
{
    /* limpiar bitmap */
    memset(p->bitmap, 0, (p->dict_size + 7) / 8);

    /* reconstruir bitmap */
    for (uint32_t i = 0; i < p->count; ++i)
        p->bitmap[p->items[i].entry_id >> 3] |=
            (1u << (p->items[i].entry_id & 7));

    /* reconstruir heap */
    for (uint32_t i = 0; i < p->count; ++i)
        p->heap[i] = i;

    p->heap_size = p->count;

    /* heapify */
    for (int32_t i = (int32_t)p->heap_size / 2 - 1;
         i >= 0;
         --i)
    {
        /* sift down manual inline */
        uint32_t idx = (uint32_t)i;

        while (1) {
            uint32_t l = (idx << 1) + 1;
            uint32_t r = l + 1;
            uint32_t s = idx;

            if (l < p->heap_size &&
                p->items[p->heap[l]].due <
                p->items[p->heap[s]].due)
                s = l;

            if (r < p->heap_size &&
                p->items[p->heap[r]].due <
                p->items[p->heap[s]].due)
                s = r;

            if (s == idx) break;

            uint32_t tmp = p->heap[idx];
            p->heap[idx] = p->heap[s];
            p->heap[s] = tmp;

            idx = s;
        }
    }
}

/* ─────────────────────────────────────────────────────────────
 *  Snapshot format (v2)
 *
 *  [event_count]
 *  [card_count]
 *  [srs_item array]
 * ───────────────────────────────────────────────────────────── */

static bool save_snapshot(FILE *fp,
                          const srs_profile *p,
                          uint32_t event_count)
{
    rewind(fp);

    fwrite(&event_count, sizeof(event_count), 1, fp);
    fwrite(&p->count, sizeof(p->count), 1, fp);
    fwrite(p->items, sizeof(srs_item), p->count, fp);

    fflush(fp);
    return true;
}

static bool load_snapshot(FILE *fp,
                          srs_profile *p,
                          uint32_t dict_size,
                          uint32_t *out_event_count)
{
    rewind(fp);

    if (fread(out_event_count,
              sizeof(*out_event_count),
              1, fp) != 1)
        return false;

    uint32_t count;
    if (fread(&count,
              sizeof(count),
              1, fp) != 1)
        return false;

    if (!srs_init(p, dict_size))
        return false;

    if (count > p->capacity) {
        p->capacity = count;
        p->items = realloc(p->items,
                           p->capacity * sizeof(srs_item));
        p->heap  = realloc(p->heap,
                           p->capacity * sizeof(uint32_t));
    }

    if (fread(p->items,
              sizeof(srs_item),
              count, fp) != count)
        return false;

    p->count = count;

    rebuild_runtime_structures(p);

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
    if (!s->events_fp)
        return false;

    s->snapshot_fp = fopen(snapshot_path, "rb+");
    if (!s->snapshot_fp)
        s->snapshot_fp = fopen(snapshot_path, "wb+");

    fseek(s->events_fp, 0, SEEK_END);
    long sz = ftell(s->events_fp);
    s->event_count =
        (uint32_t)(sz / sizeof(srs_event));

    s->next_seq = 1;
    return true;
}

void srs_sync_close(srs_sync *s)
{
    if (s->events_fp) fclose(s->events_fp);
    if (s->snapshot_fp) fclose(s->snapshot_fp);
}

/* ─────────────────────────────────────────────────────────────
 *  Append helpers
 * ───────────────────────────────────────────────────────────── */

static bool append_event(srs_sync *s,
                         const srs_event *e)
{
    if (!write_event(s->events_fp, e))
        return false;

    fflush(s->events_fp);
    s->event_count++;
    return true;
}

/* ─────────────────────────────────────────────────────────────
 *  Public API
 * ───────────────────────────────────────────────────────────── */

bool srs_sync_add(srs_sync *s,
                  srs_profile *p,
                  uint32_t entry_id,
                  uint64_t now)
{
    if (!srs_add(p, entry_id, now))
        return false;

    srs_event e = {
        .timestamp = now,
        .entry_id  = entry_id,
        .device_id = s->device_id,
        .seq       = s->next_seq++,
        .type      = SRS_EVT_ADD
    };

    return append_event(s, &e);
}

bool srs_sync_remove(srs_sync *s,
                     srs_profile *p,
                     uint32_t entry_id,
                     uint64_t now)
{
    if (!srs_remove(p, entry_id))
        return false;

    srs_event e = {
        .timestamp = now,
        .entry_id  = entry_id,
        .device_id = s->device_id,
        .seq       = s->next_seq++,
        .type      = SRS_EVT_REMOVE
    };

    return append_event(s, &e);
}

bool srs_sync_review(srs_sync *s,
                     srs_profile *p,
                     uint32_t entry_id,
                     srs_quality q,
                     uint64_t now)
{
    for (uint32_t i = 0; i < p->count; ++i) {
        if (p->items[i].entry_id == entry_id) {

            srs_answer(&p->items[i], q, now);

            /* importante: requeue */
            p->heap[p->heap_size++] = i;

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

bool srs_sync_rebuild(srs_sync *s,
                      srs_profile *p,
                      uint32_t dict_size)
{
    uint32_t snap_events = 0;

    if (!load_snapshot(s->snapshot_fp,
                       p,
                       dict_size,
                       &snap_events))
    {
        srs_init(p, dict_size);
        snap_events = 0;
    }

    fseek(s->events_fp,
          snap_events * sizeof(srs_event),
          SEEK_SET);

    srs_event e;

    while (fread(&e,
                 sizeof(e),
                 1,
                 s->events_fp) == 1)
    {
        switch (e.type) {

        case SRS_EVT_ADD:
            srs_add(p,
                    e.entry_id,
                    e.timestamp);
            break;

        case SRS_EVT_REMOVE:
            srs_remove(p,
                       e.entry_id);
            break;

        case SRS_EVT_REVIEW:
            for (uint32_t i = 0;
                 i < p->count;
                 ++i)
            {
                if (p->items[i].entry_id
                    == e.entry_id)
                {
                    srs_answer(&p->items[i],
                               (srs_quality)e.quality,
                               e.timestamp);
                    break;
                }
            }
            break;
        }
    }

    rebuild_runtime_structures(p);

    if (s->event_count - snap_events
        >= SRS_SNAPSHOT_INTERVAL)
    {
        save_snapshot(s->snapshot_fp,
                      p,
                      s->event_count);
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
