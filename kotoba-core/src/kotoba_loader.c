#include "kotoba_loader.h"
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef _WIN32

static int map_file(const char *path, const uint8_t **base, uint32_t *size)
{
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD sz = GetFileSize(hFile, NULL);
    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap)
        return 0;

    *base = (const uint8_t *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    *size = sz;
    return *base != NULL;
}

static void unmap_file(const uint8_t *base)
{
    if (base)
        UnmapViewOfFile(base);
}

#else

static int map_file(const char *path, const uint8_t **base, uint32_t *size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;

    struct stat st;
    fstat(fd, &st);
    *size = st.st_size;

    *base = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return *base != MAP_FAILED;
}

static void unmap_file(const uint8_t *base, uint32_t size)
{
    if (base)
        munmap((void *)base, size);
}

#endif

int kotoba_loader_open(kotoba_loader *l,
                       const char *bin_path,
                       const char *idx_path)
{
    memset(l, 0, sizeof(*l));

    /* map BIN */
    if (!map_file(bin_path, &l->bin_base, &l->bin_size))
        return 0;

    l->bin_hdr = (const kotoba_bin_header *)l->bin_base;

    if (l->bin_hdr->magic != KOTOBA_MAGIC)
        return 0;

    /* map IDX */
    if (!map_file(idx_path, &l->idx_base, &l->idx_size))
        return 0;

    l->idx_hdr = (const kotoba_idx_header *)l->idx_base;

    if (l->idx_hdr->magic != KOTOBA_MAGIC)
        return 0;

    /* índice empieza justo después del header */
    l->index = (const entry_index *)(l->idx_base +
                                     sizeof(kotoba_idx_header));

    return 1;
}

int kotoba_loader_get(const kotoba_loader *l,
                      uint32_t i,
                      entry_view *out)
{
    if (i >= l->bin_hdr->entry_count)
        return 0;

    uint32_t off = l->index[i].offset;

    if (off >= l->bin_size)
        return 0;

    const entry_bin *eb =
        (const entry_bin *)(l->bin_base + off);

    out->base = (const uint8_t *)eb;
    out->eb   = eb;

    return 1;
}

void kotoba_loader_close(kotoba_loader *l)
{
#ifdef _WIN32
    unmap_file(l->bin_base);
    unmap_file(l->idx_base);
#else
    unmap_file(l->bin_base, l->bin_size);
    unmap_file(l->idx_base, l->idx_size);
#endif

    memset(l, 0, sizeof(*l));
}
