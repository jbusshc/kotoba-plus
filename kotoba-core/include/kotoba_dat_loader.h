#ifndef KOTOBA_DAT_LOADER_H
#define KOTOBA_DAT_LOADER_H

#include "kotoba_dat.h"
#include "api.h"
#include <string.h>

#define DAT_ROOT 1

#ifdef __cplusplus
extern "C"
{
#endif

    KOTOBA_API int kotoba_dat_load(kotoba_dat *d, const kotoba_file *file);
    KOTOBA_API void kotoba_dat_unload(kotoba_dat *d);

    KOTOBA_API int kotoba_dat_search(const kotoba_dat *d, const int *codes, int len);
    KOTOBA_API int kotoba_dat_prefix_node(const kotoba_dat *d, const int *codes, int len);
#ifdef __cplusplus
}
#endif

#endif // KOTOBA_DAT_LOADER_H