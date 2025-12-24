#include "kotoba/loader.h"
#include <string.h>

/* ============================================================================
 *  Helpers plataforma
 * ============================================================================
 */

int kotoba_file_open(kotoba_file *f, const char *path)
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

void kotoba_file_close(kotoba_file *f)
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

int kotoba_dict_open(kotoba_dict *d,
                     const char *bin_path,
                     const char *idx_path)
{
    memset(d, 0, sizeof(*d));

    if (!kotoba_file_open(&d->bin, bin_path))
        return 0;

    if (!kotoba_file_open(&d->idx, idx_path))
        return 0;

    /* Validar headers */

    const kotoba_bin_header *bh =
        (const kotoba_bin_header *)d->bin.base;

    const kotoba_idx_header *ih =
        (const kotoba_idx_header *)d->idx.base;

    if (bh->magic != KOTOBA_MAGIC ||
        ih->magic != KOTOBA_MAGIC)
        return 0;

    if (bh->version != KOTOBA_VERSION ||
        ih->version != KOTOBA_IDX_VERSION)
        return 0;
    
    if (bh->entry_count != ih->entry_count)
        return 0;

    if (ih->entry_stride != sizeof(entry_index))
        return 0;

    d->entry_count = bh->entry_count;

    d->table = (const entry_index *)(
        (const uint8_t *)d->idx.base +
        sizeof(kotoba_idx_header));

    return 1;
}

void kotoba_dict_close(kotoba_dict *d)
{
    kotoba_file_close(&d->bin);
    kotoba_file_close(&d->idx);
    memset(d, 0, sizeof(*d));
}

/* ============================================================================
 *  Acceso a entries
 * ============================================================================
 */

 const entry_bin *
kotoba_dict_get_entry(const kotoba_dict *d, uint32_t i)
{
    if (i >= d->entry_count)
        return NULL;

    uint32_t off = d->table[i].offset;
    return (const entry_bin *)(
        (const uint8_t *)d->bin.base + off);
}
