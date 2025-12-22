#include "kotoba_viewer.h"

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
    if (i >= d->entry_count)
        return NULL;

    return (const entry_bin *)((uint8_t *)d->bin.base + d->table[i].offset);
}

/* ============================================================================
 * k_ele
 * ============================================================================
 */
const k_ele_bin *kotoba_k_ele(const kotoba_dict *d,
                              const entry_bin *e,
                              uint32_t i)
{
    if (i >= e->k_elements_count)
        return NULL;

    return (const k_ele_bin *)((uint8_t *)d->bin.base + e->k_elements_off) + i;
}

kotoba_str kotoba_keb(const kotoba_dict *d,
                      const k_ele_bin *k)
{
    return kotoba_view_string(d->bin.base, k->keb_off);
}

/* ============================================================================
 * r_ele
 * ============================================================================
 */
const r_ele_bin *
kotoba_r_ele(const kotoba_dict *d,
             const entry_bin *e,
             uint32_t i)
{
    if (i >= e->r_elements_count)
        return NULL;

    return (const r_ele_bin *)((uint8_t *)d->bin.base + e->r_elements_off) + i;
}

kotoba_str
kotoba_reb(const kotoba_dict *d,
           const r_ele_bin *r)
{
    return kotoba_view_string(d->bin.base, r->reb_off);
}

/* ============================================================================
 * sense
 * ============================================================================
 */
const sense_bin *
kotoba_sense(const kotoba_dict *d,
             const entry_bin *e,
             uint32_t i)
{
    if (i >= e->senses_count)
        return NULL;

    return (const sense_bin *)((uint8_t *)d->bin.base + e->senses_off) + i;
}

kotoba_str
kotoba_gloss(const kotoba_dict *d,
             const sense_bin *s,
             uint32_t i)
{
    if (i >= s->gloss_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->gloss_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}
kotoba_str
kotoba_stagk(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->stagk_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->stagk_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_stagr(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->stagr_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->stagr_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_pos(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->pos_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->pos_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_xref(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->xref_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->xref_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_ant(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->ant_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->ant_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_field(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->field_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->field_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_misc(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->misc_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->misc_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_s_inf(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->s_inf_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->s_inf_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_lsource(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->lsource_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->lsource_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

kotoba_str
kotoba_dial(const kotoba_dict *d, const sense_bin *s, uint32_t i)
{
    if (i >= s->dial_count)
        return (kotoba_str){0};

    const uint32_t *offs =
        (const uint32_t *)((uint8_t *)d->bin.base + s->dial_off);

    return kotoba_view_string(d->bin.base, offs[i]);
}

