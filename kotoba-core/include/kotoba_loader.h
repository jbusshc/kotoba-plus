#ifndef KOTOBA_LOADER_H
#define KOTOBA_LOADER_H

#include <stdint.h>
#include "kotoba_types.h"
#include "api.h"

typedef struct {
    /* BIN */
    const uint8_t           *bin_base;
    uint32_t                 bin_size;
    const kotoba_bin_header *bin_hdr;

    /* IDX */
    const uint8_t           *idx_base;
    uint32_t                 idx_size;
    const kotoba_idx_header *idx_hdr;
    const entry_index       *index;
} kotoba_loader;

KOTOBA_API int  kotoba_loader_open(kotoba_loader *l,
                                  const char *bin_path,
                                  const char *idx_path);

KOTOBA_API void kotoba_loader_close(kotoba_loader *l);

KOTOBA_API int  kotoba_loader_get(const kotoba_loader *l,
                                 uint32_t i,
                                 entry_view *out);

#endif
