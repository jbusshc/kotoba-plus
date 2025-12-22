#ifndef KOTOBA_DAT_BUILDER_H
#define KOTOBA_DAT_BUILDER_H

#include <stdint.h>

#include "api.h"

typedef struct
{
    int32_t *base;
    int32_t *check;
    int32_t *value;

    uint32_t size;   /* nodos usados */
    uint32_t cap;    /* capacidad arrays */
} dat_builder;

/* lifecycle */
KOTOBA_API void dat_builder_init(dat_builder *b);
KOTOBA_API void dat_builder_destroy(dat_builder *b);

/* inserción */
KOTOBA_API int  dat_builder_insert(dat_builder *b,
                         const int *codes,
                         int len,
                         int value);

/* util */
KOTOBA_API void dat_builder_compact(dat_builder *b);

#endif
