#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H

#include <stdint.h>
#include <stdio.h>
#include "kotoba_types.h"
#include "api.h"

/* Escribe el header global del archivo */
KOTOBA_API int kotoba_write_header(FILE *fp, kotoba_bin_header *header);

/* Escribe un entry y devuelve offset absoluto */
KOTOBA_API uint32_t write_entry(FILE *fp, const entry *e);

/* Entry + índice */
KOTOBA_API uint32_t write_entry_with_index(FILE *fp_bin,
                                          FILE *fp_idx,
                                          const entry *e);

#endif
