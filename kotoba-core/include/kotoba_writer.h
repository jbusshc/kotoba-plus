#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H


#include "kotoba_types.h"
#include "api.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 *  Writer state
 * ============================================================================
 */

typedef struct
{
    FILE *bin;
    FILE *idx;

    uint32_t entry_count;
} kotoba_writer;




KOTOBA_API int kotoba_writer_open(kotoba_writer *w,
                       const char *bin_path,
                       const char *idx_path);

KOTOBA_API int kotoba_writer_write_entry(kotoba_writer *w, const entry *e);

KOTOBA_API void kotoba_writer_close(kotoba_writer *w);


#endif
