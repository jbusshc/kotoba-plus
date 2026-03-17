#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H

#include "types.h"
#include "kotoba.h"
#include "loader.h"

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

struct kotoba_dict;

KOTOBA_API int kotoba_writer_open(kotoba_writer *w,
                       const char *bin_path,
                       const char *idx_path);

KOTOBA_API int kotoba_writer_open_patch(kotoba_writer *w,
                       const char *bin_path,
                       const char *idx_path,
                       const kotoba_dict *dict);

KOTOBA_API int kotoba_writer_write_entry(kotoba_writer *w, const entry *e);
KOTOBA_API int kotoba_writer_patch_entries(kotoba_writer *w, const kotoba_dict *dict);

KOTOBA_API void kotoba_writer_close(kotoba_writer *w);




#endif
