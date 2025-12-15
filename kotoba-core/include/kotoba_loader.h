#ifndef KOTOBA_LOADER_H
#define KOTOBA_LOADER_H

#include <stdint.h>
#include "kotoba_types.h"
#include "api.h"


/* Valida header global */
KOTOBA_API int kotoba_check_header(const uint8_t *file, uint32_t magic, uint16_t version);

/* Carga un entry */
KOTOBA_API int entry_load(const uint8_t *file,
                          uint32_t offset,
                          entry_view *out);

/* helpers */
const char *entry_get_string(const entry_view *, uint32_t);
const uint32_t *entry_offsets(const entry_view *, uint32_t);

const k_ele_bin *entry_k_ele(const entry_view *, uint32_t);
const r_ele_bin *entry_r_ele(const entry_view *, uint32_t);
const sense_bin *entry_sense(const entry_view *, uint32_t);

#endif
