#include "loader.h"
#include <string.h>

/* =========================
   mmap
   ========================= */

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
    if (!GetFileSizeEx(f->file, &size)) {
        CloseHandle(f->file);
        return 0;
    }

    f->size = (size_t)size.QuadPart;

    f->mapping = CreateFileMappingA(
        f->file, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!f->mapping) {
        CloseHandle(f->file);
        return 0;
    }

    f->base = MapViewOfFile(
        f->mapping, FILE_MAP_READ, 0, 0, 0);

    if (!f->base) {
        CloseHandle(f->mapping);
        CloseHandle(f->file);
        return 0;
    }

#else
    f->fd = open(path, O_RDONLY);
    if (f->fd < 0)
        return 0;

    struct stat st;
    if (fstat(f->fd, &st) != 0) {
        close(f->fd);
        return 0;
    }

    f->size = (size_t)st.st_size;

    if (f->size == 0) {
        close(f->fd);
        return 0;
    }

    f->base = mmap(NULL, f->size,
                   PROT_READ, MAP_SHARED,
                   f->fd, 0);

    if (f->base == MAP_FAILED) {
        close(f->fd);
        return 0;
    }
#endif

    return 1;
}

void kotoba_file_close(kotoba_file *f)
{
#ifdef _WIN32
    if (f->base) UnmapViewOfFile(f->base);
    if (f->mapping) CloseHandle(f->mapping);
    if (f->file && f->file != INVALID_HANDLE_VALUE) CloseHandle(f->file);
#else
    if (f->base && f->base != MAP_FAILED)
        munmap(f->base, f->size);

    if (f->fd >= 0)
        close(f->fd);
#endif

    memset(f, 0, sizeof(*f));
}

/* =========================
   HASH
   ========================= */

static inline uint32_t hash_u32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

static inline uint32_t probe_distance(uint32_t hash,
                                      uint32_t slot,
                                      uint32_t mask)
{
    return (slot + mask + 1 - (hash & mask)) & mask;
}

static int hash_find(const kotoba_dict *d,
                     uint32_t key,
                     uint32_t *out_index)
{
    uint32_t mask = d->hash_header->capacity - 1;

    uint32_t hash = hash_u32(key);
    uint32_t pos = hash & mask;
    uint32_t dist = 0;

    while (1)
    {
        const rh_entry *cur = &d->hash_entries[pos];

        if (cur->ent_seq == 0)
            return 0;

#if RH_STORE_HASH
        uint32_t cur_hash = cur->hash;
#else
        uint32_t cur_hash = hash_u32(cur->ent_seq);
#endif

        uint32_t cur_dist = probe_distance(cur_hash, pos, mask);

        if (cur_dist < dist)
            return 0;

        if (cur->ent_seq == key)
        {
            *out_index = cur->index;
            return 1;
        }

        pos = (pos + 1) & mask;
        dist++;
    }
}

/* =========================
   OPEN
   ========================= */

static int is_power_of_two(uint32_t x)
{
    return x && ((x & (x - 1)) == 0);
}

int kotoba_dict_open(kotoba_dict *d,
                     const char *bin_path,
                     const char *idx_path)
{
    memset(d, 0, sizeof(*d));

    if (!kotoba_file_open(&d->bin, bin_path))
        return 0;

    if (!kotoba_file_open(&d->idx, idx_path)) {
        kotoba_file_close(&d->bin);
        return 0;
    }

    if (d->bin.size < sizeof(kotoba_bin_header)) {
        kotoba_dict_close(d);
        return 0;
    }

    d->bin_header = (const kotoba_bin_header *)d->bin.base;

    if (d->bin_header->magic != KOTOBA_MAGIC ||
        d->bin_header->entry_count == 0) {
        kotoba_dict_close(d);
        return 0;
    }

    size_t needed =
        sizeof(kotoba_bin_header) +
        (size_t)d->bin_header->entry_count * sizeof(entry_bin);

    if (needed > d->bin.size) {
        kotoba_dict_close(d);
        return 0;
    }

    d->entries = (const entry_bin *)(
        (const uint8_t *)d->bin.base +
        sizeof(kotoba_bin_header));

    if (d->idx.size < sizeof(rh_header)) {
        kotoba_dict_close(d);
        return 0;
    }

    d->hash_header = (const rh_header *)d->idx.base;

    if (d->hash_header->magic != KOTOBA_MAGIC ||
        !is_power_of_two(d->hash_header->capacity)) {
        kotoba_dict_close(d);
        return 0;
    }

    size_t expected =
        sizeof(rh_header) +
        (size_t)d->hash_header->capacity * sizeof(rh_entry);

    if (expected != d->idx.size) {
        kotoba_dict_close(d);
        return 0;
    }

    if (d->hash_header->count != d->bin_header->entry_count) {
        kotoba_dict_close(d);
        return 0;
    }

    d->hash_entries = (const rh_entry *)(
        (const uint8_t *)d->idx.base + sizeof(rh_header));

    return 1;
}

/* =========================
   ACCESS
   ========================= */

const entry_bin *
kotoba_dict_get_entry(const kotoba_dict *d, uint32_t i)
{
    if (!d || !d->bin_header)
        return NULL;

    if (i >= d->bin_header->entry_count)
        return NULL;

    return &d->entries[i];
}

const entry_bin *
kotoba_dict_get_entry_by_entseq(const kotoba_dict *d,
                                uint32_t ent_seq)
{
    uint32_t index;

    if (!d || !d->hash_header)
        return NULL;

    if (!hash_find(d, ent_seq, &index))
        return NULL;

    if (index >= d->bin_header->entry_count)
        return NULL;

    return &d->entries[index];
}

void kotoba_dict_close(kotoba_dict *d)
{
    kotoba_file_close(&d->bin);
    kotoba_file_close(&d->idx);
    memset(d, 0, sizeof(*d));
}