#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H

#include <stdint.h>
#include <stdio.h>
#include "kotoba_types.h"
#include "api.h"

/* Escribe el header global del archivo */
KOTOBA_API int kotoba_write_bin_header(FILE *fp,
                                      uint32_t entry_count);

KOTOBA_API int kotoba_write_idx_header(FILE *fp,
                                      uint32_t entry_count);


/* Escribe un entry y devuelve offset absoluto */
KOTOBA_API uint32_t write_entry(FILE *fp, const entry *e);

/* Entry + índice */
KOTOBA_API uint32_t write_entry_with_index(FILE *fp_bin,
                                          FILE *fp_idx,
                                          const entry *e);

#endif
