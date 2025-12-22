#include "kotoba_kana_offsets.h"
#include <string.h>

int kana_offsets_load(kana_offsets *o,
                      const void *data)
{
    memset(o, 0, sizeof(*o));
    o->base = (const uint8_t *)data;
    return 1;
}

const kana_offset_entry *
kana_offsets_get(const kana_offsets *o, uint32_t offset)
{
    const uint8_t *p = o->base + offset;

    uint32_t count = *(const uint32_t *)p;
    const uint32_t *ids = (const uint32_t *)(p + 4);

    static kana_offset_entry e;
    e.count = count;
    e.ids   = ids;
    return &e;
}

uint32_t kana_offsets_write(FILE *f,
                            const uint32_t *ids,
                            uint32_t count)
{
    uint32_t off = (uint32_t)ftell(f);
    fwrite(&count, 4, 1, f);
    fwrite(ids, 4, count, f);
    return off;
}