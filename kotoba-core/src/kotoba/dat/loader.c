#include "kotoba/dat/loader.h"
#include <string.h>

#define DAT_ROOT 1

int kotoba_dat_load(kotoba_dat *d, const kotoba_file *file)
{
    memset(d, 0, sizeof(*d));

    const uint8_t *p = (const uint8_t *)file->base;
    const kotoba_dat_header *h = (const kotoba_dat_header *)p;

    if (h->magic != KOTOBA_DAT_MAGIC)
        return 0;

    if (h->version != KOTOBA_DAT_VERSION)
        return 0;

    d->file       = file;
    d->node_count = h->node_count;

    const uint8_t *cur = p + sizeof(*h);

    d->base  = (const int32_t *)cur;
    cur += sizeof(int32_t) * d->node_count;

    d->check = (const int32_t *)cur;
    cur += sizeof(int32_t) * d->node_count;

    d->value = (const int32_t *)cur;

    return 1;
}

void kotoba_dat_unload(kotoba_dat *d)
{
    memset(d, 0, sizeof(*d));
}

/* ============================================================================
 * Search
 * ============================================================================
 */

int kotoba_dat_search(const kotoba_dat *d,
                      const int *codes,
                      int len)
{
    int node = DAT_ROOT;

    for (int i = 0; i < len; i++)
    {
        int next = d->base[node] + codes[i];
        if (next <= 0 || next >= (int)d->node_count)
            return -1;
        if (d->check[next] != node)
            return -1;
        node = next;
    }

    return d->value[node];
}

int kotoba_dat_prefix_node(const kotoba_dat *d,
                           const int *codes,
                           int len)
{
    int node = DAT_ROOT;

    for (int i = 0; i < len; i++)
    {
        int next = d->base[node] + codes[i];
        if (next <= 0 || next >= (int)d->node_count)
            return -1;
        if (d->check[next] != node)
            return -1;
        node = next;
    }

    return node;
}
