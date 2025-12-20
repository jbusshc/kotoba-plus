#include "kotoba_loader.h"
#include <string.h>
#include <stdio.h>


int kotoba_dict_open(kotoba_dict *d,
                     const char *bin_path,
                     const char *idx_path)
{
    memset(d, 0, sizeof(*d));

    if (!kotoba_file_open(&d->bin, bin_path))
        return 0;

    if (!kotoba_file_open(&d->idx, idx_path))
        return 0;

    printf("Tamaño BIN: %zu bytes\n", d->bin.size);
    /* Validar headers */

    const kotoba_bin_header *bh =
        (const kotoba_bin_header *)d->bin.base;

    const kotoba_idx_header *ih =
        (const kotoba_idx_header *)d->idx.base;

    printf("magic check bin: %08X idx: %08X\n", bh->magic, ih->magic);
    if (bh->magic != KOTOBA_MAGIC ||
        ih->magic != KOTOBA_MAGIC)
        return 0;

    printf("version check bin: %04X idx: %04X\n", bh->version, ih->version);
    if (bh->version != KOTOBA_VERSION ||
        ih->version != KOTOBA_IDX_VERSION)
        return 0;
    
    printf("entry count check bin: %u idx: %u\n", bh->entry_count, ih->entry_count);
    if (bh->entry_count != ih->entry_count)
        return 0;

    printf("entry stride idx: %u\n", ih->entry_stride);
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
