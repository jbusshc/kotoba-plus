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
 *  Helpers plataforma
 * ============================================================================
 */

static int kotoba_file_open(kotoba_file *f, const char *path)
{
    memset(f, 0, sizeof(*f));

#ifdef _WIN32
    f->file = CreateFileA(path, GENERIC_READ,
                          FILE_SHARE_READ, NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (f->file == INVALID_HANDLE_VALUE)
        return 0;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(f->file, &size))
        return 0;

    f->size = (size_t)size.QuadPart;

    f->mapping = CreateFileMappingA(
        f->file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!f->mapping)
        return 0;

    f->base = MapViewOfFile(
        f->mapping, FILE_MAP_READ, 0, 0, 0);
    if (!f->base)
        return 0;

#else
    f->fd = open(path, O_RDONLY);
    if (f->fd < 0)
        return 0;

    struct stat st;
    if (fstat(f->fd, &st) != 0)
        return 0;

    f->size = (size_t)st.st_size;

    f->base = mmap(NULL, f->size,
                   PROT_READ, MAP_SHARED,
                   f->fd, 0);
    if (f->base == MAP_FAILED)
        return 0;
#endif

    return 1;
}

static void kotoba_file_close(kotoba_file *f)
{
#ifdef _WIN32
    if (f->base)
        UnmapViewOfFile(f->base);
    if (f->mapping)
        CloseHandle(f->mapping);
    if (f->file)
        CloseHandle(f->file);
#else
    if (f->base && f->size)
        munmap(f->base, f->size);
    if (f->fd >= 0)
        close(f->fd);
#endif

    memset(f, 0, sizeof(*f));
}

/* ============================================================================
 *  API pública
 * ============================================================================
 */

KOTOBA_API int kotoba_dict_open(kotoba_dict *d,
                        const char *bin_path,
                        const char *idx_path);

KOTOBA_API void kotoba_dict_close(kotoba_dict *d);

KOTOBA_API const entry_bin* kotoba_dict_get_entry(const kotoba_dict *d, uint32_t i);

/* ============================================================================
 *  Helpers básicos (inlineables)
 * ============================================================================
 */

static inline const void *
kotoba_ptr(const kotoba_dict *d, uint32_t off)
{
    return (const uint8_t *)d->bin.base + off;
}

static inline const char *
kotoba_string(const kotoba_dict *d, uint32_t off, uint8_t *len)
{
    const uint8_t *p = (const uint8_t *)kotoba_ptr(d, off);
    if (len)
        *len = p[0];
    return (const char *)(p + 1);
}

#endif
