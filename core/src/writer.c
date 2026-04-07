#include "writer.h"
#include "dict_tokens.h"
#include "viewer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ============================================================================
 * Helpers sobre data_buf
 *
 * Todos los "writes" durante kotoba_writer_write_entry van al data_buf
 * en memoria. Los offsets devueltos son relativos al inicio de data_buf.
 * Al cerrar, se ajustan sumando el desplazamiento del bloque de datos.
 * ============================================================================
 */

            static void fix_offset_array(uint8_t *data_buf, uint32_t data_base,
                                        uint32_t arr_off_abs, uint8_t count)
            {
                if (count == 0 || arr_off_abs == 0) return;

                uint32_t arr_rel = arr_off_abs - data_base;
                for (uint32_t k = 0; k < count; ++k)
                {
                    uint32_t v;
                    memcpy(&v, data_buf + arr_rel + k * 4, 4);
                    if (v != 0xFFFFFFFF) v += data_base;
                    memcpy(data_buf + arr_rel + k * 4, &v, 4);
                }
            }

static int data_buf_grow(kotoba_writer *w, uint32_t extra)
{
    uint32_t needed = w->data_len + extra;
    if (needed <= w->data_cap)
        return 1;

    uint32_t new_cap = w->data_cap ? w->data_cap * 2 : (1u << 23); /* 8 MB inicial */
    while (new_cap < needed)
        new_cap *= 2;

    uint8_t *p = (uint8_t *)realloc(w->data_buf, new_cap);
    if (!p) return 0;
    w->data_buf = p;
    w->data_cap = new_cap;
    return 1;
}

/* Devuelve offset relativo al inicio de data_buf */
static uint32_t db_tell(const kotoba_writer *w)
{
    return w->data_len;
}

static int db_write(kotoba_writer *w, const void *src, uint32_t n)
{
    if (!data_buf_grow(w, n)) return 0;
    memcpy(w->data_buf + w->data_len, src, n);
    w->data_len += n;
    return 1;
}

static int db_write_u8(kotoba_writer *w, uint8_t v)
{
    return db_write(w, &v, 1);
}

static int db_write_u32(kotoba_writer *w, uint32_t v)
{
    return db_write(w, &v, 4);
}

/* Escribe [u8 len][bytes] y devuelve offset relativo */
static uint32_t db_write_string(kotoba_writer *w, const char *s)
{
    if (w->data_len == 0)
        db_write_u8(w, 0);  // dummy para que ningún string válido empiece en offset 0
    uint32_t off = db_tell(w);
    uint8_t  len = (uint8_t)strlen(s);
    db_write_u8(w, len);
    db_write(w, s, len);
    return off;
}

static uint32_t db_write_bytes(kotoba_writer *w, const uint8_t *buf, uint32_t n)
{
    uint32_t off = db_tell(w);
    db_write(w, buf, n);
    return off;
}

static uint32_t db_write_u32_array(kotoba_writer *w,
                                   const uint32_t *buf, uint32_t n)
{
    uint32_t off = db_tell(w);
    for (uint32_t i = 0; i < n; ++i)
        db_write_u32(w, buf[i]);
    return off;
}

static uint32_t db_write_offset_array(kotoba_writer *w,
                                      const uint32_t *offs, uint32_t n)
{
    return db_write_u32_array(w, offs, n);
}

/* Parchea 4 bytes en data_buf en una posición relativa ya conocida */
static void db_patch_u32(kotoba_writer *w, uint32_t rel_off, uint32_t v)
{
    memcpy(w->data_buf + rel_off, &v, 4);
}

/* ============================================================================
 * Robin-Hood hash (misma función que loader)
 * ============================================================================
 */

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

static void rh_insert(rh_entry *table, uint32_t capacity,
                      uint32_t ent_seq, uint32_t index)
{
    uint32_t mask = capacity - 1;
    uint32_t h    = hash_u32(ent_seq);
    uint32_t pos  = h & mask;

    rh_entry ins = { ent_seq, index
#if RH_STORE_HASH
                   , h
#endif
    };

    uint32_t dist = 0;
    while (1)
    {
        rh_entry *cur = &table[pos];
        if (cur->ent_seq == 0) { *cur = ins; return; }

#if RH_STORE_HASH
        uint32_t cur_h = cur->hash;
#else
        uint32_t cur_h = hash_u32(cur->ent_seq);
#endif
        uint32_t cur_dist = probe_distance(cur_h, pos, mask);
        if (cur_dist < dist) {
            rh_entry tmp = *cur; *cur = ins; ins = tmp;
            dist = cur_dist;
        }
        pos = (pos + 1) & mask;
        dist++;
    }
}

/* ============================================================================
 * entry_buf helpers
 * ============================================================================
 */

static int entry_buf_push(kotoba_writer *w, const entry_bin *eb)
{
    if (w->entry_count >= w->entry_cap) {
        uint32_t new_cap = w->entry_cap ? w->entry_cap * 2 : 4096;
        entry_bin *p = (entry_bin *)realloc(w->entry_buf,
                                            new_cap * sizeof(entry_bin));
        if (!p) return 0;
        w->entry_buf = p;
        w->entry_cap = new_cap;
    }
    w->entry_buf[w->entry_count++] = *eb;
    return 1;
}

/* ============================================================================
 * hash_table helpers
 * ============================================================================
 */

static uint32_t hash_capacity_for(uint32_t n)
{
    uint32_t cap = 16;
    while (cap < (uint32_t)(n * 1.25 + 1))
        cap <<= 1;
    return cap;
}

static int hash_table_maybe_grow(kotoba_writer *w)
{
    if ((w->entry_count + 1) * 10 <= w->hash_capacity * 8)
        return 1;

    uint32_t  new_cap   = w->hash_capacity * 2;
    rh_entry *new_table = (rh_entry *)calloc(new_cap, sizeof(rh_entry));
    if (!new_table) return 0;

    for (uint32_t i = 0; i < w->hash_capacity; ++i)
        if (w->hash_table[i].ent_seq != 0)
            rh_insert(new_table, new_cap,
                      w->hash_table[i].ent_seq,
                      w->hash_table[i].index);

    free(w->hash_table);
    w->hash_table    = new_table;
    w->hash_capacity = new_cap;
    return 1;
}

/* ============================================================================
 * Apertura
 * ============================================================================
 */

int kotoba_writer_open(kotoba_writer *w,
                       const char *bin_path,
                       const char *idx_path)
{
    memset(w, 0, sizeof(*w));

    w->bin = fopen(bin_path, "wb");
    if (!w->bin) return 0;

    w->idx = fopen(idx_path, "wb");
    if (!w->idx) { fclose(w->bin); return 0; }

    /* hash table inicial */
    w->hash_capacity = hash_capacity_for(RH_MAX_CAPACITY);
    w->hash_table    = (rh_entry *)calloc(w->hash_capacity, sizeof(rh_entry));
    if (!w->hash_table) {
        fclose(w->bin); fclose(w->idx);
        return 0;
    }

    /* data_buf inicial: 32 MB, suficiente para JMdict completo */
    w->data_cap = 1u << 25;
    w->data_buf = (uint8_t *)malloc(w->data_cap);
    if (!w->data_buf) {
        free(w->hash_table);
        fclose(w->bin); fclose(w->idx);
        return 0;
    }

    return 1;
}

/* ============================================================================
 * Escritura de ENTRY
 *
 * Todos los offsets calculados aquí son RELATIVOS al inicio de data_buf.
 * Se ajustan al archivo real en kotoba_writer_close.
 * ============================================================================
 */

int kotoba_writer_write_entry(kotoba_writer *w, const entry *e)
{
    entry_bin eb = {0};
    eb.ent_seq = (uint32_t)e->ent_seq;

    /* =======================================================================
     * k_ele_bin[]
     * ======================================================================= */
    if (e->k_elements_count > 0)
    {
        /* reservar array de k_ele_bin en data_buf */
        eb.k_elements_off = db_tell(w);
        k_ele_bin zero    = {0};
        for (uint32_t i = 0; i < e->k_elements_count; ++i)
            db_write(w, &zero, sizeof(zero));

        for (uint32_t i = 0; i < e->k_elements_count; ++i)
        {
            const k_ele *src = &e->k_elements[i];
            k_ele_bin kb     = {0};

            kb.keb_off = db_write_string(w, src->keb);

            if (src->ke_inf_count > 0) {
                uint8_t buf[MAX_KE_INF];
                for (uint32_t j = 0; j < src->ke_inf_count; ++j) {
                    int val = ke_inf_token_index(src->ke_inf[j]);
                    if (val == -1) { fprintf(stderr, "ke_inf desconocido '%s'\n", src->ke_inf[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                kb.ke_inf_off   = db_write_bytes(w, buf, src->ke_inf_count);
                kb.ke_inf_count = (uint8_t)src->ke_inf_count;
            }

            if (src->ke_pri_count > 0) {
                uint8_t buf[MAX_KE_PRI];
                for (uint32_t j = 0; j < src->ke_pri_count; ++j) {
                    int val = ke_pri_token_index(src->ke_pri[j]);
                    if (val == -1) { fprintf(stderr, "ke_pri desconocido '%s'\n", src->ke_pri[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                kb.ke_pri_off   = db_write_bytes(w, buf, src->ke_pri_count);
                kb.ke_pri_count = (uint8_t)src->ke_pri_count;
            }

            /* parchear el k_ele_bin reservado */
            db_patch_u32(w, eb.k_elements_off + i * sizeof(k_ele_bin)
                            + offsetof(k_ele_bin, keb_off),     kb.keb_off);
            db_patch_u32(w, eb.k_elements_off + i * sizeof(k_ele_bin)
                            + offsetof(k_ele_bin, ke_inf_off),  kb.ke_inf_off);
            db_patch_u32(w, eb.k_elements_off + i * sizeof(k_ele_bin)
                            + offsetof(k_ele_bin, ke_pri_off),  kb.ke_pri_off);
            memcpy(w->data_buf + eb.k_elements_off + i * sizeof(k_ele_bin),
                   &kb, sizeof(kb));
        }
        eb.k_elements_count = (uint8_t)e->k_elements_count;
    }

    /* =======================================================================
     * r_ele_bin[]
     * ======================================================================= */
    if (e->r_elements_count > 0)
    {
        eb.r_elements_off = db_tell(w);
        r_ele_bin zero    = {0};
        for (uint32_t i = 0; i < e->r_elements_count; ++i)
            db_write(w, &zero, sizeof(zero));

        for (uint32_t i = 0; i < e->r_elements_count; ++i)
        {
            const r_ele *src = &e->r_elements[i];
            r_ele_bin rb     = {0};

            rb.reb_off = db_write_string(w, src->reb);

            if (src->re_restr_count > 0) {
                uint8_t buf[MAX_RE_RESTR];
                for (uint32_t j = 0; j < src->re_restr_count; ++j) {
                    int keb_idx = -1;
                    for (uint32_t k = 0; k < e->k_elements_count; ++k)
                        if (strcmp(src->re_restr[j], e->k_elements[k].keb) == 0)
                            { keb_idx = k; break; }
                    if (keb_idx == -1) { fprintf(stderr, "re_restr '%s' no encontrado\n", src->re_restr[j]); buf[j] = 0xFF; }
                    else buf[j] = (uint8_t)keb_idx;
                }
                rb.re_restr_off   = db_write_bytes(w, buf, src->re_restr_count);
                rb.re_restr_count = (uint8_t)src->re_restr_count;
            }

            if (src->re_inf_count > 0) {
                uint8_t buf[MAX_RE_INF];
                for (uint32_t j = 0; j < src->re_inf_count; ++j) {
                    int val = re_inf_token_index(src->re_inf[j]);
                    if (val == -1) { fprintf(stderr, "re_inf desconocido '%s'\n", src->re_inf[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                rb.re_inf_off   = db_write_bytes(w, buf, src->re_inf_count);
                rb.re_inf_count = (uint8_t)src->re_inf_count;
            }

            if (src->re_pri_count > 0) {
                uint8_t buf[MAX_RE_PRI];
                for (uint32_t j = 0; j < src->re_pri_count; ++j) {
                    int val = re_pri_token_index(src->re_pri[j]);
                    if (val == -1) { fprintf(stderr, "re_pri desconocido '%s'\n", src->re_pri[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                rb.re_pri_off   = db_write_bytes(w, buf, src->re_pri_count);
                rb.re_pri_count = (uint8_t)src->re_pri_count;
            }

            rb.re_nokanji = src->re_nokanji ? 1 : 0;

            memcpy(w->data_buf + eb.r_elements_off + i * sizeof(r_ele_bin),
                   &rb, sizeof(rb));
        }
        eb.r_elements_count = (uint8_t)e->r_elements_count;
    }

    /* =======================================================================
     * sense_bin[]
     * ======================================================================= */
    if (e->senses_count > 0)
    {
        eb.senses_off  = db_tell(w);
        sense_bin zero = {0};
        for (uint32_t i = 0; i < e->senses_count; ++i)
            db_write(w, &zero, sizeof(zero));

        for (uint32_t i = 0; i < e->senses_count; ++i)
        {
            const sense *src = &e->senses[i];
            sense_bin sb     = {0};

            if (src->dial_count > 0) {
                uint8_t buf[MAX_DIAL];
                for (uint32_t j = 0; j < src->dial_count; ++j) {
                    int val = dial_token_index(src->dial[j]);
                    if (val == -1) { fprintf(stderr, "dial desconocido '%s'\n", src->dial[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                sb.dial_off   = db_write_bytes(w, buf, src->dial_count);
                sb.dial_count = (uint8_t)src->dial_count;
            }

            if (src->field_count > 0) {
                uint8_t buf[MAX_FIELD];
                for (uint32_t j = 0; j < src->field_count; ++j) {
                    int val = field_token_index(src->field[j]);
                    if (val == -1) { fprintf(stderr, "field desconocido '%s'\n", src->field[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                sb.field_off   = db_write_bytes(w, buf, src->field_count);
                sb.field_count = (uint8_t)src->field_count;
            }

            if (src->misc_count > 0) {
                uint8_t buf[MAX_MISC];
                for (uint32_t j = 0; j < src->misc_count; ++j) {
                    int val = misc_token_index(src->misc[j]);
                    if (val == -1) { fprintf(stderr, "misc desconocido '%s'\n", src->misc[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                sb.misc_off   = db_write_bytes(w, buf, src->misc_count);
                sb.misc_count = (uint8_t)src->misc_count;
            }

            if (src->pos_count > 0) {
                uint8_t buf[MAX_POS];
                for (uint32_t j = 0; j < src->pos_count; ++j) {
                    int val = pos_token_index(src->pos[j]);
                    if (val == -1) { fprintf(stderr, "pos desconocido '%s'\n", src->pos[j]); val = 0xFF; }
                    buf[j] = (uint8_t)val;
                }
                sb.pos_off   = db_write_bytes(w, buf, src->pos_count);
                sb.pos_count = (uint8_t)src->pos_count;
            }

            if (src->stagk_count > 0) {
                uint8_t buf[MAX_STAGK];
                for (uint32_t j = 0; j < src->stagk_count; ++j) {
                    int val = -1;
                    for (uint32_t k = 0; k < e->k_elements_count; ++k)
                        if (strcmp(src->stagk[j], e->k_elements[k].keb) == 0)
                            { val = k; break; }
                    if (val == -1) { fprintf(stderr, "stagk '%s' no encontrado\n", src->stagk[j]); buf[j] = 0xFF; }
                    else buf[j] = (uint8_t)val;
                }
                sb.stagk_off   = db_write_bytes(w, buf, src->stagk_count);
                sb.stagk_count = (uint8_t)src->stagk_count;
            }

            if (src->stagr_count > 0) {
                uint8_t buf[MAX_STAGR];
                for (uint32_t j = 0; j < src->stagr_count; ++j) {
                    int val = -1;
                    for (uint32_t k = 0; k < e->r_elements_count; ++k)
                        if (strcmp(src->stagr[j], e->r_elements[k].reb) == 0)
                            { val = k; break; }
                    if (val == -1) { fprintf(stderr, "stagr '%s' no encontrado\n", src->stagr[j]); buf[j] = 0xFF; }
                    else buf[j] = (uint8_t)val;
                }
                sb.stagr_off   = db_write_bytes(w, buf, src->stagr_count);
                sb.stagr_count = (uint8_t)src->stagr_count;
            }

            if (src->xref_count > 0) {
                uint32_t buf[MAX_XREF];
                for (uint32_t j = 0; j < src->xref_count; ++j)
                    buf[j] = 0xFFFFFFFF;
                sb.xref_off   = db_write_u32_array(w, buf, src->xref_count);
                sb.xref_count = (uint8_t)src->xref_count;
            }

            if (src->ant_count > 0) {
                uint32_t buf[MAX_ANT];
                for (uint32_t j = 0; j < src->ant_count; ++j)
                    buf[j] = 0xFFFFFFFF;
                sb.ant_off   = db_write_u32_array(w, buf, src->ant_count);
                sb.ant_count = (uint8_t)src->ant_count;
            }

            if (src->s_inf_count > 0) {
                uint32_t offs[MAX_S_INF];
                for (uint32_t j = 0; j < src->s_inf_count; ++j)
                    offs[j] = db_write_string(w, src->s_inf[j]);
                sb.s_inf_off   = db_write_offset_array(w, offs, src->s_inf_count);
                sb.s_inf_count = (uint8_t)src->s_inf_count;
            }

            if (src->lsource_count > 0) {
                uint32_t offs[MAX_LSOURCE];
                for (uint32_t j = 0; j < src->lsource_count; ++j)
                    offs[j] = db_write_string(w, src->lsource[j]);
                sb.lsource_off   = db_write_offset_array(w, offs, src->lsource_count);
                sb.lsource_count = (uint8_t)src->lsource_count;
            }

            if (src->gloss_count > 0) {
                uint32_t offs[MAX_GLOSS];
                for (uint32_t j = 0; j < src->gloss_count; ++j)
                    offs[j] = db_write_string(w, src->gloss[j]);
                sb.gloss_off   = db_write_offset_array(w, offs, src->gloss_count);
                sb.gloss_count = (uint8_t)src->gloss_count;
            }

            sb.lang = (uint8_t)src->lang;

            memcpy(w->data_buf + eb.senses_off + i * sizeof(sense_bin),
                   &sb, sizeof(sb));
        }
        eb.senses_count = (uint8_t)e->senses_count;
    }

    eb.priority = e->priority;

    /* registrar en hash */
    if (!hash_table_maybe_grow(w)) return 0;
    rh_insert(w->hash_table, w->hash_capacity,
              eb.ent_seq, w->entry_count);

    /* empujar entry_bin al buffer de entradas */
    return entry_buf_push(w, &eb);
}

/* ============================================================================
 * Cierre
 *
 * Layout final del .bin:
 *   [kotoba_bin_header]                      sizeof(kotoba_bin_header)
 *   [entry_bin × N]                          N * sizeof(entry_bin)
 *   [data_buf]                               data_len bytes
 *
 * Los offsets en entry_bin y en los sub-structs dentro de data_buf son
 * actualmente relativos al inicio de data_buf. Hay que ajustarlos sumando
 * data_base = sizeof(kotoba_bin_header) + N * sizeof(entry_bin).
 * ============================================================================
 */

/* Ajusta un uint32_t en data_buf sumando delta, si no es 0 */
static inline void fix_off(uint8_t *data, uint32_t rel_off, uint32_t delta)
{
    uint32_t v;
    memcpy(&v, data + rel_off, 4);
    if (v == 0xFFFFFFFF) return; /* offset ausente */
    v += delta;
    memcpy(data + rel_off, &v, 4);
}

/* Ajusta todos los offsets internos de un k_ele_bin en data_buf */
static void fix_k_ele(uint8_t *data, uint32_t base, uint32_t delta)
{
    fix_off(data, base + offsetof(k_ele_bin, keb_off),    delta);
    fix_off(data, base + offsetof(k_ele_bin, ke_inf_off), delta);
    fix_off(data, base + offsetof(k_ele_bin, ke_pri_off), delta);
}

static void fix_r_ele(uint8_t *data, uint32_t base, uint32_t delta)
{
    fix_off(data, base + offsetof(r_ele_bin, reb_off),       delta);
    fix_off(data, base + offsetof(r_ele_bin, re_restr_off),  delta);
    fix_off(data, base + offsetof(r_ele_bin, re_inf_off),    delta);
    fix_off(data, base + offsetof(r_ele_bin, re_pri_off),    delta);
}

static void fix_sense(uint8_t *data, uint32_t base, uint32_t delta)
{
    fix_off(data, base + offsetof(sense_bin, dial_off),    delta);
    fix_off(data, base + offsetof(sense_bin, field_off),   delta);
    fix_off(data, base + offsetof(sense_bin, misc_off),    delta);
    fix_off(data, base + offsetof(sense_bin, pos_off),     delta);
    fix_off(data, base + offsetof(sense_bin, stagk_off),   delta);
    fix_off(data, base + offsetof(sense_bin, stagr_off),   delta);
    fix_off(data, base + offsetof(sense_bin, xref_off),    delta);
    fix_off(data, base + offsetof(sense_bin, ant_off),     delta);
    fix_off(data, base + offsetof(sense_bin, s_inf_off),   delta);
    fix_off(data, base + offsetof(sense_bin, lsource_off), delta);
    fix_off(data, base + offsetof(sense_bin, gloss_off),   delta);
}

static void write_u8 (FILE *f, uint8_t  v) { fwrite(&v, 1, 1, f); }
static void write_u16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }
static void write_u32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }

void kotoba_writer_close(kotoba_writer *w)
{
    uint32_t N         = w->entry_count;
    uint32_t data_base = (uint32_t)(sizeof(kotoba_bin_header)
                                    + (size_t)N * sizeof(entry_bin));

    /* ── Ajustar offsets en entry_buf y en data_buf ── */
    for (uint32_t i = 0; i < N; ++i)
    {
        entry_bin *eb = &w->entry_buf[i];

        /* Ajustar los offsets de los arrays de sub-structs que viven en
           data_buf, y también los offsets internos de esos sub-structs. */

        if (eb->k_elements_count > 0) {
            for (uint32_t j = 0; j < eb->k_elements_count; ++j)
                fix_k_ele(w->data_buf,
                          eb->k_elements_off + j * sizeof(k_ele_bin),
                          data_base);
            eb->k_elements_off += data_base;
        }

        if (eb->r_elements_count > 0) {
            for (uint32_t j = 0; j < eb->r_elements_count; ++j)
                fix_r_ele(w->data_buf,
                          eb->r_elements_off + j * sizeof(r_ele_bin),
                          data_base);
            eb->r_elements_off += data_base;
        }

        if (eb->senses_count > 0) {
            for (uint32_t j = 0; j < eb->senses_count; ++j)
                fix_sense(w->data_buf,
                          eb->senses_off + j * sizeof(sense_bin),
                          data_base);
            eb->senses_off += data_base;
        }
    }

    /* Los offsets dentro de los offset-arrays (gloss_off, s_inf_off, etc.)
       apuntan directamente a strings en data_buf — ya fueron ajustados
       por fix_sense/fix_k_ele/fix_r_ele. Los valores almacenados en esos
       arrays (uint32_t[]) también son offsets relativos a data_buf y
       necesitan ser ajustados. */
    for (uint32_t i = 0; i < N; ++i)
    {
        entry_bin *eb = &w->entry_buf[i];

        /* Para cada sense, ajustar los uint32_t[] de offset-arrays
           (gloss, s_inf, lsource) — sus valores son offsets a strings. */
        for (uint32_t j = 0; j < eb->senses_count; ++j)
        {
            /* Después del ajuste anterior, senses_off ya incluye data_base.
               Para acceder en data_buf usamos senses_off - data_base. */
            uint32_t sb_rel = (eb->senses_off - data_base)
                              + j * sizeof(sense_bin);
            sense_bin sb;
            memcpy(&sb, w->data_buf + sb_rel, sizeof(sb));

            /* Los *_off en sb ya fueron ajustados (apuntan al archivo).
               Los valores dentro de los arrays de offsets todavía son
               relativos a data_buf — hay que ajustarlos. */



            fix_offset_array(w->data_buf, data_base, sb.gloss_off,   sb.gloss_count);
            fix_offset_array(w->data_buf, data_base, sb.s_inf_off,   sb.s_inf_count);
            fix_offset_array(w->data_buf, data_base, sb.lsource_off, sb.lsource_count);
        }
    }

    /* ── Escribir .bin ── */
    /* header */
    write_u32(w->bin, KOTOBA_MAGIC);
    write_u32(w->bin, N);
    write_u16(w->bin, KOTOBA_VERSION);
    write_u16(w->bin, KOTOBA_FILE_BIN);

    /* array contiguo de entry_bin */
    fwrite(w->entry_buf, sizeof(entry_bin), N, w->bin);

    /* datos variables */
    fwrite(w->data_buf, 1, w->data_len, w->bin);

    fclose(w->bin);
    w->bin = NULL;

    /* ── Escribir .idx (robin-hood hash table) ── */
    write_u32(w->idx, KOTOBA_MAGIC);
    write_u32(w->idx, w->hash_capacity);
    write_u32(w->idx, N);
    write_u16(w->idx, KOTOBA_IDX_VERSION);
    write_u16(w->idx, KOTOBA_FILE_IDX);

    for (uint32_t i = 0; i < w->hash_capacity; ++i) {
        write_u32(w->idx, w->hash_table[i].ent_seq);
        write_u32(w->idx, w->hash_table[i].index);
#if RH_STORE_HASH
        write_u32(w->idx, w->hash_table[i].hash);
#endif
    }

    fclose(w->idx);
    w->idx = NULL;

    free(w->entry_buf);  w->entry_buf  = NULL;
    free(w->data_buf);   w->data_buf   = NULL;
    free(w->hash_table); w->hash_table = NULL;
}

/* ============================================================================
 * Patch de xrefs/ants
 *
 * Igual que antes. Trabaja sobre el .bin ya volcado al disco (modo r+b).
 * Los offsets en sense_bin ya son absolutos (ajustados en close).
 * ============================================================================
 */

int kotoba_writer_patch_entries(kotoba_writer *w, const kotoba_dict *dict)
{
    FILE *f = fopen("./patch.tsv", "r");
    if (!f) { perror("fopen patch.tsv"); return 0; }

    char line[4096];
    while (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\r\n")] = 0;

        char *id_str = strtok(line, "\t");
        if (!id_str) continue;
        uint32_t entry_id = (uint32_t)atoi(id_str);

        char *sense_id_str = strtok(NULL, "\t");
        if (!sense_id_str) continue;
        uint32_t sense_id = (uint32_t)atoi(sense_id_str);

        char *xref_count_str = strtok(NULL, "\t");
        if (!xref_count_str) continue;
        uint32_t xref_count = (uint32_t)atoi(xref_count_str);

        const entry_bin *target_e = kotoba_dict_get_entry(dict, entry_id);
        if (!target_e) continue;
        const sense_bin *target_s = kotoba_sense(dict, target_e, sense_id);
        if (!target_s) continue;

                /* helper: busca entry por keb o reb */
        #define FIND_ENTRY(primary_, secondary_, out_idx_) do {             \
            (out_idx_) = -1;                                                 \
            for (uint32_t m = 0; m < dict->bin_header->entry_count && (out_idx_) == -1; ++m) { \
                const entry_bin *xe = kotoba_dict_get_entry(dict, m);        \
                /* Revisar KEBs */                                           \
                for (uint32_t n = 0; n < xe->k_elements_count; ++n) {       \
                    const k_ele_bin *k = kotoba_k_ele(dict, xe, n);         \
                    kotoba_str ks = kotoba_keb(dict, k);                     \
                    if (ks.len == strlen(primary_) && memcmp(ks.ptr, primary_, ks.len) == 0) { \
                        (out_idx_) = m; break;                                \
                    }                                                         \
                }                                                             \
                /* Revisar REBs */                                            \
                if ((out_idx_) == -1) {                                      \
                    for (uint32_t n = 0; n < xe->r_elements_count; ++n) {   \
                        const r_ele_bin *r = kotoba_r_ele(dict, xe, n);      \
                        kotoba_str rs = kotoba_reb(dict, r);                 \
                        if ((rs.len == strlen(primary_) && memcmp(rs.ptr, primary_, rs.len) == 0) || \
                            (secondary_[0] && rs.len == strlen(secondary_) && memcmp(rs.ptr, secondary_, rs.len) == 0)) { \
                            (out_idx_) = m; break;                            \
                        }                                                     \
                    }                                                         \
                }                                                             \
            }                                                                 \
        } while(0)

        /* ── XREFS ── */
        for (uint32_t i = 0; i < xref_count && i < 16; ++i)
        {
            char *xref_str = strtok(NULL, "\t");
            if (!xref_str) break;
            while (isspace((unsigned char)*xref_str)) xref_str++;

            char primary[256]={0}, secondary[256]={0};
            char *sep1 = strstr(xref_str, "・");
            if (sep1) {
                size_t l1 = sep1 - xref_str; if (l1>255) l1=255;
                memcpy(primary, xref_str, l1);
                char *rest = sep1 + strlen("・");
                char *sep2 = strstr(rest, "・");
                size_t l2 = sep2 ? (size_t)(sep2-rest) : strlen(rest); if (l2>255) l2=255;
                memcpy(secondary, rest, l2);
            } else { strncpy(primary, xref_str, 255); }

            int idx; FIND_ENTRY(primary, secondary, idx);

            if (idx != -1) {
                long off = (long)target_s->xref_off + i * sizeof(uint32_t);
                fseek(w->bin, off, SEEK_SET);
                write_u32(w->bin, (uint32_t)idx);
            } else {
                fprintf(stderr, "Error: xref '%s' no encontrado (entry %u sense %u)\n",
                        xref_str, entry_id, sense_id);
            }
        }

        /* ── ANTS ── */
        char *ant_count_str = strtok(NULL, "\t");
        if (!ant_count_str) continue;
        uint32_t ant_count = (uint32_t)atoi(ant_count_str);

        for (uint32_t i = 0; i < ant_count && i < 16; ++i)
        {
            char *ant_str = strtok(NULL, "\t");
            if (!ant_str) break;
            while (isspace((unsigned char)*ant_str)) ant_str++;

            char primary[256]={0}, secondary[256]={0};
            char *sep1 = strstr(ant_str, "・");
            if (sep1) {
                size_t l1 = sep1 - ant_str; if (l1>255) l1=255;
                memcpy(primary, ant_str, l1);
                char *rest = sep1 + strlen("・");
                char *sep2 = strstr(rest, "・");
                size_t l2 = sep2 ? (size_t)(sep2-rest) : strlen(rest); if (l2>255) l2=255;
                memcpy(secondary, rest, l2);
            } else { strncpy(primary, ant_str, 255); }

            int idx; FIND_ENTRY(primary, secondary, idx);

            if (idx != -1) {
                long off = (long)target_s->ant_off + i * sizeof(uint32_t);
                fseek(w->bin, off, SEEK_SET);
                write_u32(w->bin, (uint32_t)idx);
            } else {
                fprintf(stderr, "Error: ant '%s' no encontrado (entry %u sense %u)\n",
                        ant_str, entry_id, sense_id);
            }
        }

        #undef FIND_ENTRY
    }

    fclose(f);
    return 1;
}

/* ============================================================================
 * Apertura para patch (modo r+b, sin recrear el .idx)
 * ============================================================================
 */

int kotoba_writer_open_patch(kotoba_writer *w,
                             const char *bin_path,
                             const char *idx_path,
                             const kotoba_dict *dict)
{
    memset(w, 0, sizeof(*w));

    w->bin = fopen(bin_path, "r+b");
    if (!w->bin) return 0;

    w->idx = fopen(idx_path, "r+b");
    if (!w->idx) { fclose(w->bin); return 0; }

    w->entry_count = dict->bin_header->entry_count;
    return 1;
}