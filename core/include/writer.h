#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H

#include <stdint.h>
#include <stdio.h>
#include "kotoba.h"
#include "types.h"
#include "loader.h"

/* ============================================================================
 * WRITER
 *
 * Estrategia de escritura (two-pass en memoria):
 *
 *   El loader espera el .bin con este layout:
 *
 *     [kotoba_bin_header]
 *     [entry_bin × N]          <- array CONTIGUO, accedido con &d->entries[i]
 *     [datos variables...]     <- strings, arrays de k_ele_bin, r_ele_bin, etc.
 *
 *   El writer acumula dos buffers en memoria:
 *     - entry_buf : array dinámico de entry_bin (crece con cada entrada)
 *     - data_buf  : buffer de datos variables (strings, sub-arrays, etc.)
 *
 *   Los offsets dentro de entry_bin (k_elements_off, r_elements_off, etc.)
 *   se calculan relativos al inicio de data_buf durante la escritura.
 *   Al cerrar, se ajustan sumando (sizeof(header) + N*sizeof(entry_bin))
 *   antes de volcarlos al archivo.
 *
 * ============================================================================
 */

typedef struct {
    /* archivos de salida */
    FILE    *bin;
    FILE    *idx;

    /* buffer de entry_bin (array contiguo en memoria) */
    entry_bin *entry_buf;
    uint32_t   entry_count;
    uint32_t   entry_cap;

    /* buffer de datos variables */
    uint8_t  *data_buf;
    uint32_t  data_len;
    uint32_t  data_cap;

    /* tabla hash robin-hood en memoria, volcada al cerrar */
    rh_entry *hash_table;
    uint32_t  hash_capacity;

} kotoba_writer;

/* ============================================================================
 * API
 * ============================================================================
 */

int  kotoba_writer_open(kotoba_writer *w,
                        const char *bin_path,
                        const char *idx_path);

int  kotoba_writer_write_entry(kotoba_writer *w,
                               const entry *e);

int  kotoba_writer_patch_entries(kotoba_writer *w,
                                 const kotoba_dict *dict);

void kotoba_writer_close(kotoba_writer *w);

int  kotoba_writer_open_patch(kotoba_writer *w,
                              const char *bin_path,
                              const char *idx_path,
                              const kotoba_dict *dict);

#endif