#ifndef KOTOBA_DAT_COLLECT_H
#define KOTOBA_DAT_COLLECT_H

#include "kotoba_dat.h"
#include "api.h"

#define KOTOBA_COLLECT_MAX 1024

typedef struct
{
    int values[KOTOBA_COLLECT_MAX];
    int count;
} kotoba_collect_result;

/* prefix + collect */
KOTOBA_API int kotoba_dat_collect(const kotoba_dat *d,
                       const int *codes,
                       int len,
                       kotoba_collect_result *out);

#endif
