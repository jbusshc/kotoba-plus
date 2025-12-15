#include "kotoba_dict.h"
#include <string.h>

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

    if (d->idx.size < sizeof(kotoba_idx_header)) {
        kotoba_file_close(&d->bin);
        kotoba_file_close(&d->idx);
        return 0;
    }

    const kotoba_idx_header *hdr = (const kotoba_idx_header *)d->idx.base;

    if (hdr->version != KOTOBA_VERSION) { // usar KOTOBA_VERSION
        kotoba_file_close(&d->bin);
        kotoba_file_close(&d->idx);
        return 0;
    }

    // Suponemos que entry_count está justo después del header
    d->entry_count = hdr->entry_count;
    d->table = (const entry_index *)(d->idx.base + sizeof(kotoba_idx_header));


    return 1;
}

void kotoba_dict_close(kotoba_dict *d)
{
    kotoba_file_close(&d->bin);
    kotoba_file_close(&d->idx);
}

int kotoba_dict_get(const kotoba_dict *d,
                    uint32_t i,
                    entry_view *out)
{
    if (!d || !out || i >= d->entry_count) return 0;

    const entry_index *idx = &d->table[i];

    if (idx->offset + sizeof(entry_bin) > d->bin.size - sizeof(kotoba_bin_header))
        return 0;

    const uint8_t *entry_ptr = d->bin.base + sizeof(kotoba_bin_header) + idx->offset;

    out->base = entry_ptr;
    out->eb   = (const entry_bin *)entry_ptr;

    return 1;
}

uint32_t kotoba_dict_entry_count(const kotoba_dict *d)
{
    return d ? d->entry_count : 0;
}
