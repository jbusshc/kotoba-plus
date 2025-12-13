#ifndef KOTOBA_LOADER_H
#define KOTOBA_LOADER_H

#include "kotoba_types.h"
#include <stdint.h>

typedef struct {
    void     *map;
    uint64_t  size;

    uint32_t        entry_count;
    const entry_index *index;
    const uint8_t  *entries_base;
} kotoba_db;

int  kotoba_open(kotoba_db *db, const char *path);
void kotoba_close(kotoba_db *db);

static inline const entry_bin *
kotoba_get_entry(const kotoba_db *db, uint32_t i) {
    return (const entry_bin *)(db->entries_base + db->index[i].offset);
}

#endif
