#ifndef KOTOBA_KANA_OFFSETS_H
#define KOTOBA_KANA_OFFSETS_H

#include <stdint.h>
#include <stdio.h>
#include "api.h"

typedef struct
{
    uint32_t count;
    const uint32_t *ids;
} kana_offset_entry;

/* loader */
typedef struct
{
    const uint8_t *base;
    uint32_t entry_count;
} kana_offsets;

KOTOBA_API int kana_offsets_load(kana_offsets *o,
                                 const void *data);

KOTOBA_API const kana_offset_entry *
kana_offsets_get(const kana_offsets *o, uint32_t offset);

KOTOBA_API uint32_t kana_offsets_write(FILE *f,
                                       const uint32_t *ids,
                                       uint32_t count);

#endif
