#include "kotoba/dat/builder.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DAT_INITIAL_CAP 1024
#define DAT_ROOT 1

/* ------------------------------------------------------------
 * helpers
 * ------------------------------------------------------------ */

static void dat_ensure(dat_builder *b, uint32_t need)
{
    if (need < b->cap)
        return;

    uint32_t new_cap = b->cap;
    while (new_cap <= need)
        new_cap *= 2;

    b->base  = realloc(b->base,  new_cap * sizeof(int32_t));
    b->check = realloc(b->check, new_cap * sizeof(int32_t));
    b->value = realloc(b->value, new_cap * sizeof(int32_t));

    for (uint32_t i = b->cap; i < new_cap; ++i)
    {
        b->base[i]  = 0;
        b->check[i] = 0;
        b->value[i] = -1;
    }

    b->cap = new_cap;
}

static int dat_find_base(dat_builder *b,
                         const int *codes,
                         int count)
{
    int base = 1;

    for (;; ++base)
    {
        int ok = 1;
        for (int i = 0; i < count; ++i)
        {
            int t = base + codes[i];
            dat_ensure(b, t + 1);

            if (b->check[t] != 0)
            {
                ok = 0;
                break;
            }
        }
        if (ok)
            return base;
    }
}

/* ------------------------------------------------------------
 * lifecycle
 * ------------------------------------------------------------ */

void dat_builder_init(dat_builder *b)
{
    memset(b, 0, sizeof(*b));

    b->cap = DAT_INITIAL_CAP;
    b->size = DAT_ROOT + 1;

    b->base  = calloc(b->cap, sizeof(int32_t));
    b->check = calloc(b->cap, sizeof(int32_t));
    b->value = malloc(b->cap * sizeof(int32_t));

    for (uint32_t i = 0; i < b->cap; ++i)
        b->value[i] = -1;

    /* root */
    b->base[DAT_ROOT]  = 1;
    b->check[DAT_ROOT] = -1;
}

void dat_builder_destroy(dat_builder *b)
{
    free(b->base);
    free(b->check);
    free(b->value);
    memset(b, 0, sizeof(*b));
}

/* ------------------------------------------------------------
 * insertion
 * ------------------------------------------------------------ */

int dat_builder_insert(dat_builder *b,
                       const int *codes,
                       int len,
                       int val)
{
    int s = DAT_ROOT;

    for (int i = 0; i < len; ++i)
    {
        int c = codes[i];
        assert(c > 0);

        int t = b->base[s] + c;
        dat_ensure(b, t + 1);

        if (b->check[t] == s)
        {
            s = t;
            continue;
        }

        if (b->check[t] == 0)
        {
            b->check[t] = s;
            b->base[t]  = 1;
            s = t;
            continue;
        }

        /* conflicto → rebase */
        int siblings[256];
        int sib_count = 0;

        for (uint32_t i2 = 1; i2 < b->cap; ++i2)
        {
            if (b->check[i2] == s)
                siblings[sib_count++] = i2 - b->base[s];
        }

        siblings[sib_count++] = c;

        int new_base = dat_find_base(b, siblings, sib_count);

        /* mover hijos */
        for (int j = 0; j < sib_count - 1; ++j)
        {
            int old = b->base[s] + siblings[j];
            int neu = new_base + siblings[j];

            dat_ensure(b, neu + 1);

            b->base[neu]  = b->base[old];
            b->check[neu] = s;
            b->value[neu] = b->value[old];

            /* limpiar viejo */
            b->check[old] = 0;
            b->base[old]  = 0;
            b->value[old] = -1;
        }

        b->base[s] = new_base;

        /* insertar nuevo */
        t = new_base + c;
        b->check[t] = s;
        b->base[t]  = 1;
        s = t;
    }

    b->value[s] = val;
    if ((uint32_t)s >= b->size)
        b->size = s + 1;

    return 1;
}

/* ------------------------------------------------------------
 * compaction (opcional)
 * ------------------------------------------------------------ */

void dat_builder_compact(dat_builder *b)
{
    /* opcional: reducir cap → size */
    if (b->cap == b->size)
        return;

    b->base  = realloc(b->base,  b->size * sizeof(int32_t));
    b->check = realloc(b->check, b->size * sizeof(int32_t));
    b->value = realloc(b->value, b->size * sizeof(int32_t));

    b->cap = b->size;
}
