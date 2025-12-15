#ifndef KOTOBA_DICT_H
#define KOTOBA_DICT_H

#include "kotoba_types.h"
#include "kotoba_file.h"
#include "api.h"


typedef struct {
    kotoba_file bin;
    kotoba_file idx;

    const entry_index *table;
    uint32_t entry_count;
} kotoba_dict;

KOTOBA_API int kotoba_dict_open(kotoba_dict *d,
                     const char *bin_path,
                     const char *idx_path);

KOTOBA_API void kotoba_dict_close(kotoba_dict *d);
KOTOBA_API int kotoba_dict_get(const kotoba_dict *d,
                    uint32_t i,
                    entry_view *out);

KOTOBA_API uint32_t kotoba_dict_entry_count(const kotoba_dict *d);
#endif
