#ifndef KOTOBA_LOADER_H
#define KOTOBA_LOADER_H

#include <stdint.h>
#include "kotoba_types.h"
#include "api.h"

#include "kotoba_types.h"
#include "api.h"

#include <string.h>
#include <assert.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/* ============================================================================
 *  Archivo mmapeado
 * ============================================================================
 */

typedef struct
{
    void   *base;
    size_t  size;

#ifdef _WIN32
    HANDLE file;
    HANDLE mapping;
#else
    int fd;
#endif
} kotoba_file;

/* ============================================================================
 *  Diccionario cargado
 * ============================================================================
 */

typedef struct
{
    kotoba_file bin;
    kotoba_file idx;

    const entry_index *table;
    uint32_t entry_count;
} kotoba_dict;



/* ============================================================================
 *  API pública
 * ============================================================================
 */

KOTOBA_API int kotoba_file_open(kotoba_file *f, const char *path);
KOTOBA_API void kotoba_file_close(kotoba_file *f);

KOTOBA_API int kotoba_dict_open(kotoba_dict *d,
                        const char *bin_path,
                        const char *idx_path);

KOTOBA_API void kotoba_dict_close(kotoba_dict *d);

KOTOBA_API const entry_bin* kotoba_dict_get_entry(const kotoba_dict *d, uint32_t i);



#endif
