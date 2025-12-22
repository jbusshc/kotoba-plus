#include "kotoba_dat_collect.h"
#include <string.h>

typedef struct
{
    int node;
} stack_item;

int kotoba_dat_collect(const kotoba_dat *d,
                       const int *codes,
                       int len,
                       kotoba_collect_result *out)
{
    memset(out, 0, sizeof(*out));

    int start = kotoba_dat_prefix_node(d, codes, len);
    if (start < 0)
        return 0;

    stack_item stack[KOTOBA_COLLECT_MAX];
    int sp = 0;

    stack[sp++].node = start;

    while (sp > 0)
    {
        int node = stack[--sp].node;

        if (d->value[node] >= 0)
        {
            if (out->count < KOTOBA_COLLECT_MAX)
                out->values[out->count++] = d->value[node];
        }

        /* recorrer hijos */
        for (uint32_t i = 1; i < d->node_count; ++i)
        {
            if (d->check[i] == node)
            {
                if (sp < KOTOBA_COLLECT_MAX)
                    stack[sp++].node = i;
            }
        }
    }

    return out->count;
}
