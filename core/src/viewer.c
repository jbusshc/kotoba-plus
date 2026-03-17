#include "viewer.h"
#include <string.h>
#include <stdint.h>

#define INDEX_ERROR 0xFFFFFFFF

/* ================================
 * Acceso a string [len][bytes]
 * ================================ */
static inline kotoba_str
kotoba_view_string(const uint8_t *base, uint32_t off)
{
    kotoba_str s;
    s.len = base[off];
    s.ptr = (const char *)(base + off + 1);
    return s;
}

/* ============================================================================
 * Entry
 * ============================================================================
 */
const entry_bin *kotoba_entry(const kotoba_dict *d, uint32_t i)
{
    if (i >= d->entry_count) return NULL;
    return (const entry_bin *)((uint8_t *)d->bin.base + d->table[i].offset);
}

/* ============================================================================
 * k_ele
 * ============================================================================
 */
const k_ele_bin *kotoba_k_ele(const kotoba_dict *d, const entry_bin *e, uint32_t i)
{
    if (i >= e->k_elements_count) return NULL;
    return (const k_ele_bin *)((uint8_t *)d->bin.base + e->k_elements_off) + i;
}

kotoba_str kotoba_keb(const kotoba_dict *d, const k_ele_bin *k)
{
    return kotoba_view_string(d->bin.base, k->keb_off);
}

/* ============================================================================
 * r_ele
 * ============================================================================
 */
const r_ele_bin *kotoba_r_ele(const kotoba_dict *d, const entry_bin *e, uint32_t i)
{
    if (i >= e->r_elements_count) return NULL;
    return (const r_ele_bin *)((uint8_t *)d->bin.base + e->r_elements_off) + i;
}

kotoba_str kotoba_reb(const kotoba_dict *d, const r_ele_bin *r)
{
    return kotoba_view_string(d->bin.base, r->reb_off);
}

/* ============================================================================
 * sense
 * ============================================================================
 */
const sense_bin *kotoba_sense(const kotoba_dict *d, const entry_bin *e, uint32_t i)
{
    if (i >= e->senses_count) return NULL;
    return (const sense_bin *)((uint8_t *)d->bin.base + e->senses_off) + i;
}

/* ================================
 * Textos libres
 * ================================ */
kotoba_str kotoba_gloss(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->gloss_count) return (kotoba_str){0};
    const uint32_t *offs = (const uint32_t *)((uint8_t *)d->bin.base + s->gloss_off);
    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str kotoba_stagk(const kotoba_dict *d, const entry_bin *e, uint8_t s_i, uint8_t k_idx)
{
    const sense_bin *s = kotoba_sense(d, e, s_i);
    if (!s) return (kotoba_str){0};
    if (k_idx >= s->stagk_count) return (kotoba_str){0};
    // access 8-bit index array to get offset of keb
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + s->stagk_off);
    uint8_t keb_idx = idxs[k_idx];
    if (keb_idx >= e->k_elements_count) return (kotoba_str){0};
    const k_ele_bin *k = kotoba_k_ele(d, e, keb_idx);
    return kotoba_keb(d, k);
}



kotoba_str kotoba_stagr(const kotoba_dict *d, const entry_bin *e, uint8_t s_i, uint8_t r_idx)
{
    const sense_bin *s = kotoba_sense(d, e, s_i);
    if (!s) return (kotoba_str){0};
    if (r_idx >= s->stagr_count) return (kotoba_str){0};
    // access 8-bit index array to get offset of reb
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + s->stagr_off);
    uint8_t reb_idx = idxs[r_idx];
    if (reb_idx >= e->r_elements_count) return (kotoba_str){0};
    const r_ele_bin *r = kotoba_r_ele(d, e, reb_idx);
    return kotoba_reb(d, r);
}

uint32_t kotoba_xref(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->xref_count) return INDEX_ERROR;
    const uint32_t *offs = (const uint32_t *)((uint8_t *)d->bin.base + s->xref_off);
    return offs[i];
}

uint32_t kotoba_ant(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->ant_count) return INDEX_ERROR;
    const uint32_t *offs = (const uint32_t *)((uint8_t *)d->bin.base + s->ant_off);
    return offs[i];
}

kotoba_str kotoba_s_inf(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->s_inf_count) return (kotoba_str){0};
    const uint32_t *offs = (const uint32_t *)((uint8_t *)d->bin.base + s->s_inf_off);
    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str kotoba_lsource(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->lsource_count) return (kotoba_str){0};
    const uint32_t *offs = (const uint32_t *)((uint8_t *)d->bin.base + s->lsource_off);
    return kotoba_view_string(d->bin.base, offs[i]);
}

/* ================================
 * Tokenizados (devuelven const char* directo)
 * ================================ */
const char* kotoba_pos(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->pos_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + s->pos_off);
    return pos_token_str(idxs[i]);
}

const char* kotoba_ke_inf(const kotoba_dict *d, const k_ele_bin *k, uint32_t i)
{
    if (i >= k->ke_inf_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + k->ke_inf_off);
    return ke_inf_token_str(idxs[i]);
}

const char* kotoba_re_inf(const kotoba_dict *d, const r_ele_bin *r, uint32_t i)
{
    if (i >= r->re_inf_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + r->re_inf_off);
    return re_inf_token_str(idxs[i]);
}

const char* kotoba_field(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->field_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + s->field_off);
    return field_token_str(idxs[i]);
}

const char* kotoba_misc(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->misc_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + s->misc_off);
    return misc_token_str(idxs[i]);
}

const char* kotoba_dial(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->dial_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + s->dial_off);
    return dial_token_str(idxs[i]);
}

const char* kotoba_ke_pri(const kotoba_dict *d, const k_ele_bin *k, uint32_t i)
{
    if (i >= k->ke_pri_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + k->ke_pri_off);
    return ke_pri_token_str(idxs[i]);
}

const char* kotoba_re_pri(const kotoba_dict *d, const r_ele_bin *r, uint32_t i)
{
    if (i >= r->re_pri_count) return NULL;
    const uint8_t* idxs = (const uint8_t *)((uint8_t *)d->bin.base + r->re_pri_off);
    return re_pri_token_str(idxs[i]);
}