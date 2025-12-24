#ifndef KOTOBA_DAT_H
#define KOTOBA_DAT_H

#include <stdint.h>
#include "../../kotoba.h"
#include "../loader.h"

/* ============================================================================
 * DAT binary header
 * ============================================================================
 */

#define KOTOBA_DAT_MAGIC   0x44415400 /* "DAT\0" */
#define KOTOBA_DAT_VERSION 1

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t node_count;
} kotoba_dat_header;

/* ============================================================================
 * DAT loaded (read-only)
 * ============================================================================
 */

typedef struct
{
    const kotoba_file *file;

    uint32_t node_count;
    const int32_t *base;
    const int32_t *check;
    const int32_t *value;
} kotoba_dat;

/* ============================================================================
 * API
 * ============================================================================
 */

/* loader */
KOTOBA_API int  kotoba_dat_load(kotoba_dat *d, const kotoba_file *file);
KOTOBA_API void kotoba_dat_unload(kotoba_dat *d);

/* search */
KOTOBA_API int kotoba_dat_search(const kotoba_dat *d,
                                 const int *codes,
                                 int len);

/* prefix node */
KOTOBA_API int kotoba_dat_prefix_node(const kotoba_dat *d,
                                      const int *codes,
                                      int len);

#endif /* KOTOBA_DAT_H */
