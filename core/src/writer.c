#include "writer.h"
#include <string.h>

/* ============================================================================
 * Helpers binarios
 * ============================================================================
 */

static uint32_t file_tell(FILE *f)
{
    return (uint32_t)ftell(f);
}

static void write_u8(FILE *f, uint8_t v)   { fwrite(&v, 1, 1, f); }
static void write_u16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }
static void write_u32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }

/* string: [u8 len][bytes] */
static uint32_t write_string(FILE *f, const char *s)
{
    uint32_t off = file_tell(f);
    uint8_t len = (uint8_t)strlen(s);
    write_u8(f, len);
    fwrite(s, 1, len, f);
    return off;
}

/* array de uint32 offsets */
static uint32_t write_offset_array(FILE *f,
                                   const uint32_t *offs,
                                   uint32_t count)
{
    uint32_t off = file_tell(f);
    for (uint32_t i = 0; i < count; ++i)
        write_u32(f, offs[i]);
    return off;
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
    if (!w->bin)
        return 0;

    w->idx = fopen(idx_path, "wb");
    if (!w->idx)
        return 0;

    /* BIN header (parche luego) */
    write_u32(w->bin, KOTOBA_MAGIC);
    write_u32(w->bin, 0); /* entry_count */
    write_u16(w->bin, KOTOBA_VERSION);
    write_u16(w->bin, KOTOBA_FILE_BIN);

    /* IDX header */
    write_u32(w->idx, KOTOBA_MAGIC);
    write_u32(w->idx, 0); /* entry_count */
    write_u32(w->idx, sizeof(entry_index));
    write_u16(w->idx, KOTOBA_IDX_VERSION);
    write_u16(w->idx, KOTOBA_FILE_IDX);

    return 1;
}

/* ============================================================================
 * Escritura de ENTRY
 * ============================================================================
 */

int kotoba_writer_write_entry(kotoba_writer *w, const entry *e)
{
    /* registrar offset en IDX */
    write_u32(w->idx, file_tell(w->bin));

    uint32_t entry_pos = file_tell(w->bin);

    /* reservar entry_bin */
    entry_bin eb = {0};
    fwrite(&eb, sizeof(eb), 1, w->bin);

    uint32_t k_off = 0, r_off = 0, s_off = 0;

    /* =======================================================================
     * k_ele_bin[]
     * ======================================================================= */
    if (e->k_elements_count > 0)
    {
        k_off = file_tell(w->bin);

        k_ele_bin zero = {0};
        for (uint32_t i = 0; i < e->k_elements_count; ++i)
            fwrite(&zero, sizeof(zero), 1, w->bin);

        for (uint32_t i = 0; i < e->k_elements_count; ++i)
        {
            const k_ele *src = &e->k_elements[i];
            k_ele_bin kb = {0};

            kb.keb_off = write_string(w->bin, src->keb);

            if (src->ke_inf_count > 0)
            {
                uint32_t offs[MAX_KE_INF];
                for (uint32_t j = 0; j < src->ke_inf_count; ++j)
                    offs[j] = write_string(w->bin, src->ke_inf[j]);

                kb.ke_inf_off   = write_offset_array(w->bin, offs, src->ke_inf_count);
                kb.ke_inf_count = (uint8_t)src->ke_inf_count;
            }

            if (src->ke_pri_count > 0)
            {
                uint32_t offs[MAX_KE_PRI];
                for (uint32_t j = 0; j < src->ke_pri_count; ++j)
                    offs[j] = write_string(w->bin, src->ke_pri[j]);

                kb.ke_pri_off   = write_offset_array(w->bin, offs, src->ke_pri_count);
                kb.ke_pri_count = (uint8_t)src->ke_pri_count;
            }

            long cur = ftell(w->bin);
            fseek(w->bin, k_off + i * sizeof(k_ele_bin), SEEK_SET);
            fwrite(&kb, sizeof(kb), 1, w->bin);
            fseek(w->bin, cur, SEEK_SET);
        }
    }

    /* =======================================================================
     * r_ele_bin[]
     * ======================================================================= */
    if (e->r_elements_count > 0)
    {
        r_off = file_tell(w->bin);

        r_ele_bin zero = {0};
        for (uint32_t i = 0; i < e->r_elements_count; ++i)
            fwrite(&zero, sizeof(zero), 1, w->bin);

        for (uint32_t i = 0; i < e->r_elements_count; ++i)
        {
            const r_ele *src = &e->r_elements[i];
            r_ele_bin rb = {0};

            rb.reb_off = write_string(w->bin, src->reb);

            if (src->re_restr_count > 0)
            {
                uint32_t offs[MAX_RE_RESTR];
                for (uint32_t j = 0; j < src->re_restr_count; ++j)
                    offs[j] = write_string(w->bin, src->re_restr[j]);

                rb.re_restr_off   = write_offset_array(w->bin, offs, src->re_restr_count);
                rb.re_restr_count = (uint8_t)src->re_restr_count;
            }

            long cur = ftell(w->bin);
            fseek(w->bin, r_off + i * sizeof(r_ele_bin), SEEK_SET);
            fwrite(&rb, sizeof(rb), 1, w->bin);
            fseek(w->bin, cur, SEEK_SET);
        }
    }

    /* =======================================================================
     * sense_bin[]
     * ======================================================================= */
    if (e->senses_count > 0)
    {
        s_off = file_tell(w->bin);

        sense_bin zero = {0};
        for (uint32_t i = 0; i < e->senses_count; ++i)
            fwrite(&zero, sizeof(zero), 1, w->bin);

        for (uint32_t i = 0; i < e->senses_count; ++i)
        {
            const sense *src = &e->senses[i];
            sense_bin sb = {0};

            if (src->gloss_count > 0)
            {
                uint32_t offs[MAX_GLOSS];
                for (uint32_t j = 0; j < src->gloss_count; ++j)
                    offs[j] = write_string(w->bin, src->gloss[j]);

                sb.gloss_off   = write_offset_array(w->bin, offs, src->gloss_count);
                sb.gloss_count = (uint8_t)src->gloss_count;
            }

            sb.lang = (uint8_t)src->lang;

            long cur = ftell(w->bin);
            fseek(w->bin, s_off + i * sizeof(sense_bin), SEEK_SET);
            fwrite(&sb, sizeof(sb), 1, w->bin);
            fseek(w->bin, cur, SEEK_SET);
        }
    }

    /* =======================================================================
     * parchear entry_bin
     * ======================================================================= */
    long end = ftell(w->bin);
    fseek(w->bin, entry_pos, SEEK_SET);

    eb.ent_seq = (uint32_t)e->ent_seq;
    eb.k_elements_off   = k_off;
    eb.r_elements_off   = r_off;
    eb.senses_off       = s_off;
    eb.k_elements_count = (uint8_t)e->k_elements_count;
    eb.r_elements_count = (uint8_t)e->r_elements_count;
    eb.senses_count     = (uint8_t)e->senses_count;
    eb.priority         = e->priority;

    fwrite(&eb, sizeof(eb), 1, w->bin);
    fseek(w->bin, end, SEEK_SET);

    w->entry_count++;
    return 1;
}

/* ============================================================================
 * Cierre
 * ============================================================================
 */

void kotoba_writer_close(kotoba_writer *w)
{
    /* BIN header */
    fseek(w->bin, 4, SEEK_SET);
    write_u32(w->bin, w->entry_count);

    /* IDX header */
    fseek(w->idx, 4, SEEK_SET);
    write_u32(w->idx, w->entry_count);

    fclose(w->bin);
    fclose(w->idx);
}
