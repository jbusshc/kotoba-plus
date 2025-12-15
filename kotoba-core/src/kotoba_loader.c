#include "kotoba_loader.h"

/* =========================================================
 * Header global
 * ========================================================= */

int kotoba_check_header(const uint8_t *file, uint32_t magic, uint16_t version)
{
    const kotoba_bin_header *h = (const kotoba_bin_header *)file;

    if (h->magic != magic) return 0;
    if (h->version != version) return 0;
    return 1;
}

/* =========================================================
 * Entry load
 * ========================================================= */

int entry_load(const uint8_t *file, uint32_t off, entry_view *out)
{
    if (!file || !out) return 0;

    out->base = file + off;
    out->eb   = (const entry_bin *)out->base;
    return 1;
}

/* =========================================================
 * Helpers
 * ========================================================= */

static inline const void *
ptr(const entry_view *v, uint32_t off)
{
    return off ? v->base + off : NULL;
}

const char *entry_get_string(const entry_view *v, uint32_t off)
{
    return (const char *)ptr(v, off);
}

const uint32_t *entry_offsets(const entry_view *v, uint32_t off)
{
    return (const uint32_t *)ptr(v, off);
}

const k_ele_bin *entry_k_ele(const entry_view *v, uint32_t i)
{
    if (i >= v->eb->k_count) return NULL;
    return &((const k_ele_bin *)(v->base + v->eb->k_off))[i];
}

const r_ele_bin *entry_r_ele(const entry_view *v, uint32_t i)
{
    if (i >= v->eb->r_count) return NULL;
    return &((const r_ele_bin *)(v->base + v->eb->r_off))[i];
}

const sense_bin *entry_sense(const entry_view *v, uint32_t i)
{
    if (i >= v->eb->s_count) return NULL;
    return &((const sense_bin *)(v->base + v->eb->s_off))[i];
}
