#ifndef KOTOBA_DAT_WRITER_H
#define KOTOBA_DAT_WRITER_H

#include <stdio.h>
#include <stdint.h>
#include "../../kotoba.h"

/* ============================================================================
 * DAT writer
 * ============================================================================
 */

typedef struct
{
    FILE *f;
    uint32_t node_count;
} kotoba_dat_writer;

/* ============================================================================
 * API
 * ============================================================================
 */

KOTOBA_API int  kotoba_dat_writer_open(kotoba_dat_writer *w,
                                      const char *path);

KOTOBA_API int  kotoba_dat_writer_write(kotoba_dat_writer *w,
                                       const int32_t *base,
                                       const int32_t *check,
                                       const int32_t *value,
                                       uint32_t node_count);

KOTOBA_API void kotoba_dat_writer_close(kotoba_dat_writer *w);

#endif /* KOTOBA_DAT_WRITER_H */
