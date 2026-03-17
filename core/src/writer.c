#include "writer.h"
#include "dict_tokens.h"
#include "viewer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


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

static uint32_t write_bytes(FILE *f, const uint8_t *buf, uint32_t count)
{
    uint32_t off = file_tell(f);
    fwrite(buf, 1, count, f);
    return off;
}

static uint32_t write_4bytes(FILE *f, const uint32_t *buf, uint32_t count)
{
    uint32_t off = file_tell(f);
    for (uint32_t i = 0; i < count; ++i)
        write_u32(f, buf[i]);
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
                uint8_t buf[MAX_KE_INF];
                for (uint32_t j = 0; j < src->ke_inf_count; ++j) {
                    int val = ke_inf_token_index(src->ke_inf[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: ke_inf desconocido '%s'\n", src->ke_inf[j]);
                        val = 0xFF; // valor inválido para indicar error
                    } 
                    buf[j] = (uint8_t)val;
                }
                kb.ke_inf_off   = write_bytes(w->bin, buf, src->ke_inf_count);
                kb.ke_inf_count = (uint8_t)src->ke_inf_count;
            }

            if (src->ke_pri_count > 0)
            {
                uint8_t buf[MAX_KE_PRI];
                for (uint32_t j = 0; j < src->ke_pri_count; ++j) {
                    int val = ke_pri_token_index(src->ke_pri[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: ke_pri desconocido '%s'\n", src->ke_pri[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                kb.ke_pri_off   = write_bytes(w->bin, buf, src->ke_pri_count);
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

            if (src->re_restr_count > 0) // array de índices de kebs; ausencia índica que aplica a todos
            {
                uint8_t buf[MAX_RE_RESTR];
                for (uint32_t j = 0; j < src->re_restr_count; ++j) {
                    // buscar índice de keb
                    int keb_idx = -1;
                    for (uint32_t k = 0; k < e->k_elements_count; ++k) {
                        if (strcmp(src->re_restr[j], e->k_elements[k].keb) == 0) {
                            keb_idx = k;
                            break;
                        }
                    }
                    if (keb_idx == -1) {
                        fprintf(stderr, "Error: re_restr '%s' no encontrado entre los kebs de la entrada\n", src->re_restr[j]);
                        buf[j] = 0xFF; // valor inválido para indicar error
                    } else {
                        buf[j] = (uint8_t)keb_idx;
                    }
                }
                rb.re_restr_off   = write_bytes(w->bin, buf, src->re_restr_count);
                rb.re_restr_count = (uint8_t)src->re_restr_count;
            }

            if (src->re_inf_count > 0)
            {
                uint8_t buf[MAX_RE_INF];
                for (uint32_t j = 0; j < src->re_inf_count; ++j) {
                    int val = re_inf_token_index(src->re_inf[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: re_inf desconocido '%s'\n", src->re_inf[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                rb.re_inf_off   = write_bytes(w->bin, buf, src->re_inf_count);
                rb.re_inf_count = (uint8_t)src->re_inf_count;
            }

            if (src->re_pri_count > 0)
            {
                uint8_t buf[MAX_RE_PRI];
                for (uint32_t j = 0; j < src->re_pri_count; ++j) {
                    int val = re_pri_token_index(src->re_pri[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: re_pri desconocido '%s'\n", src->re_pri[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                rb.re_pri_off   = write_bytes(w->bin, buf, src->re_pri_count);
                rb.re_pri_count = (uint8_t)src->re_pri_count;
            }
            if (src->re_nokanji) {
                rb.re_nokanji = 1;
            } else {
                rb.re_nokanji = 0;
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

            /* -----------------------
             * Campos tokenizados -> uint8_t indices
             * ----------------------- */
            if (src->dial_count > 0)
            {
                uint8_t buf[MAX_DIAL];
                for (uint32_t j = 0; j < src->dial_count; ++j) {
                    int val = dial_token_index(src->dial[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: dial desconocido '%s'\n", src->dial[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                sb.dial_off   = write_bytes(w->bin, buf, src->dial_count);
                sb.dial_count = (uint8_t)src->dial_count;
            }

            if (src->field_count > 0)
            {
                uint8_t buf[MAX_FIELD];
                for (uint32_t j = 0; j < src->field_count; ++j) {
                    int val = field_token_index(src->field[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: field desconocido '%s'\n", src->field[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                sb.field_off   = write_bytes(w->bin, buf, src->field_count);
                sb.field_count = (uint8_t)src->field_count;
            }

            if (src->misc_count > 0)
            {
                uint8_t buf[MAX_MISC];
                for (uint32_t j = 0; j < src->misc_count; ++j) {
                    int val = misc_token_index(src->misc[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: misc desconocido '%s'\n", src->misc[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                sb.misc_off   = write_bytes(w->bin, buf, src->misc_count);
                sb.misc_count = (uint8_t)src->misc_count;
            }

            if (src->pos_count > 0)
            {
                uint8_t buf[MAX_POS];
                for (uint32_t j = 0; j < src->pos_count; ++j) {
                    int val = pos_token_index(src->pos[j]);
                    if (val == -1) {
                        fprintf(stderr, "Error: pos desconocido '%s'\n", src->pos[j]);
                        val = 0xFF; // valor inválido para indicar error
                    }
                    buf[j] = (uint8_t)val;
                }
                sb.pos_off   = write_bytes(w->bin, buf, src->pos_count);
                sb.pos_count = (uint8_t)src->pos_count;
            }

            if (src->stagk_count > 0) //  // offset a array de índices de kebs; ausencia índica que aplica a todos
            {
                uint8_t buf[MAX_STAGK];
                for (uint32_t j = 0; j < src->stagk_count; ++j) {
                    int val = -1;
                    for (uint32_t k = 0; k < e->k_elements_count; ++k) {
                        if (strcmp(src->stagk[j], e->k_elements[k].keb) == 0) {
                            val = k;
                            break;
                        }
                    }
                    if (val == -1) {
                        fprintf(stderr, "Error: stagk '%s' no encontrado entre los kebs de la entrada\n", src->stagk[j]);
                        buf[j] = 0xFF; // valor inválido para indicar error
                    } else {
                        buf[j] = (uint8_t)val;
                    }
                }

                sb.stagk_off   = write_bytes(w->bin, buf, src->stagk_count);
                sb.stagk_count = (uint8_t)src->stagk_count;
            }

            if (src->stagr_count > 0)
            {
                uint8_t buf[MAX_STAGR];
                for (uint32_t j = 0; j < src->stagr_count; ++j) {
                    int val = -1;
                    for (uint32_t k = 0; k < e->r_elements_count; ++k) {
                        if (strcmp(src->stagr[j], e->r_elements[k].reb) == 0) {
                            val = k;
                            break;
                        }
                    }
                    if (val == -1) {
                        fprintf(stderr, "Error: stagr '%s' no encontrado entre los rebs de la entrada\n", src->stagr[j]);
                        buf[j] = 0xFF; // valor inválido para indicar error
                    } else {
                        buf[j] = (uint8_t)val;
                    }
                }
                sb.stagr_off   = write_bytes(w->bin, buf, src->stagr_count);
                sb.stagr_count = (uint8_t)src->stagr_count;
            }

            /* ENTRY INDEX PLACEHOLDER */
            if (src->xref_count > 0)
            {
                uint32_t buf[MAX_XREF];
                for (uint32_t j = 0; j < src->xref_count; ++j) {
                    buf[j] = 0xFF; // futuro índice de entrada, parcheado luego
                }
                sb.xref_off   = write_4bytes(w->bin, buf, src->xref_count);
                sb.xref_count = (uint8_t)src->xref_count;
            } 

            if (src->ant_count > 0)
            {
                uint32_t buf[MAX_ANT];
                for (uint32_t j = 0; j < src->ant_count; ++j) {
                    buf[j] = 0xFF; // futuro índice de entrada, parcheado luego
                }
                sb.ant_off   = write_4bytes(w->bin, buf, src->ant_count);
                sb.ant_count = (uint8_t)src->ant_count;
             }

            /* -----------------------
             * Campos de texto libre
             * ----------------------- */


            if (src->s_inf_count > 0)
            {
                uint32_t offs[MAX_S_INF];
                for (uint32_t j = 0; j < src->s_inf_count; ++j)
                    offs[j] = write_string(w->bin, src->s_inf[j]);
                sb.s_inf_off   = write_offset_array(w->bin, offs, src->s_inf_count);
                sb.s_inf_count = (uint8_t)src->s_inf_count;
            }

            if (src->lsource_count > 0)
            {
                uint32_t offs[MAX_LSOURCE];
                for (uint32_t j = 0; j < src->lsource_count; ++j)
                    offs[j] = write_string(w->bin, src->lsource[j]);
                sb.lsource_off   = write_offset_array(w->bin, offs, src->lsource_count);
                sb.lsource_count = (uint8_t)src->lsource_count;
            }

            if (src->gloss_count > 0)
            {
                uint32_t offs[MAX_GLOSS];
                for (uint32_t j = 0; j < src->gloss_count; ++j)
                    offs[j] = write_string(w->bin, src->gloss[j]);
                sb.gloss_off   = write_offset_array(w->bin, offs, src->gloss_count);
                sb.gloss_count = (uint8_t)src->gloss_count;
            }

            sb.lang = (uint8_t)src->lang;

            /* parchear el sense_bin reservado */
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

int kotoba_writer_patch_entries(kotoba_writer *w, const kotoba_dict *dict)
{
    FILE *f = fopen("./patch.tsv", "r");
    if (!f) {
        perror("fopen");
        return 0;
    }

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
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

        char buffer[16][256] = {{0}};

        // =========================
        // PARSE XREFS (FIX UTF-8)
        // =========================
        for (uint32_t i = 0; i < xref_count && i < 16; ++i) {
            char *xref_str = strtok(NULL, "\t");
            if (!xref_str) break;

            while (isspace((unsigned char)*xref_str)) xref_str++;

            char primary[256] = {0};
            char secondary[256] = {0};

            char *sep1 = strstr(xref_str, "・");

            if (sep1) {
                size_t len1 = sep1 - xref_str;
                if (len1 > 255) len1 = 255;
                memcpy(primary, xref_str, len1);
                primary[len1] = '\0';

                char *rest = sep1 + strlen("・");
                char *sep2 = strstr(rest, "・");

                size_t len2 = sep2 ? (size_t)(sep2 - rest) : strlen(rest);
                if (len2 > 255) len2 = 255;
                memcpy(secondary, rest, len2);
                secondary[len2] = '\0';
            } else {
                strncpy(primary, xref_str, 255);
                primary[255] = '\0';
            }

            int xref_entry_idx = -1;

            // buscar PRIMARY en keb
            for (uint32_t m = 0; m < dict->entry_count && xref_entry_idx == -1; ++m) {
                const entry_bin *xe = kotoba_entry(dict, m);

                for (uint32_t n = 0; n < xe->k_elements_count; ++n) {
                    const k_ele_bin *k = kotoba_k_ele(dict, xe, n);
                    kotoba_str ks = kotoba_keb(dict, k);

                    char buf[256];
                    uint32_t len = ks.len > 255 ? 255 : ks.len;
                    memcpy(buf, ks.ptr, len);
                    buf[len] = '\0';

                    if (strcmp(buf, primary) == 0) {
                        xref_entry_idx = m;
                        break;
                    }
                }
            }

            // buscar SECONDARY en reb (FIX: global search)
            if (xref_entry_idx == -1 && secondary[0]) {
                for (uint32_t m = 0; m < dict->entry_count && xref_entry_idx == -1; ++m) {
                    const entry_bin *xe = kotoba_entry(dict, m);

                    for (uint32_t n = 0; n < xe->r_elements_count; ++n) {
                        const r_ele_bin *r = kotoba_r_ele(dict, xe, n);
                        kotoba_str rs = kotoba_reb(dict, r);

                        char buf[256];
                        uint32_t len = rs.len > 255 ? 255 : rs.len;
                        memcpy(buf, rs.ptr, len);
                        buf[len] = '\0';

                        if (strcmp(buf, secondary) == 0) {
                            xref_entry_idx = m;
                            break;
                        }
                    }
                }
            }

            // fallback: primary en reb
            if (xref_entry_idx == -1) {
                for (uint32_t m = 0; m < dict->entry_count && xref_entry_idx == -1; ++m) {
                    const entry_bin *xe = kotoba_entry(dict, m);

                    for (uint32_t n = 0; n < xe->r_elements_count; ++n) {
                        const r_ele_bin *r = kotoba_r_ele(dict, xe, n);
                        kotoba_str rs = kotoba_reb(dict, r);

                        char buf[256];
                        uint32_t len = rs.len > 255 ? 255 : rs.len;
                        memcpy(buf, rs.ptr, len);
                        buf[len] = '\0';

                        if (strcmp(buf, primary) == 0) {
                            xref_entry_idx = m;
                            break;
                        }
                    }
                }
            }

            if (xref_entry_idx != -1) {
                long off = kotoba_sense(dict, kotoba_entry(dict, entry_id), sense_id)->xref_off
                           + i * sizeof(uint32_t);

                fseek(w->bin, off, SEEK_SET);
                write_u32(w->bin, (uint32_t)xref_entry_idx);
            } else {
                fprintf(stderr,
                        "Error: no se encontró xref '%s' para entry_id %u sense_id %u\n",
                        xref_str, entry_id, sense_id);
            }
        }

        // =========================
        // ANT (MISMO FIX)
        // =========================

        char *ant_count_str = strtok(NULL, "\t");
        if (!ant_count_str) continue;
        uint32_t ant_count = (uint32_t)atoi(ant_count_str);

        for (uint32_t i = 0; i < ant_count && i < 16; ++i) {
            char *ant_str = strtok(NULL, "\t");
            if (!ant_str) break;

            while (isspace((unsigned char)*ant_str)) ant_str++;

            char primary[256] = {0};
            char secondary[256] = {0};

            char *sep1 = strstr(ant_str, "・");

            if (sep1) {
                size_t len1 = sep1 - ant_str;
                if (len1 > 255) len1 = 255;
                memcpy(primary, ant_str, len1);
                primary[len1] = '\0';

                char *rest = sep1 + strlen("・");
                char *sep2 = strstr(rest, "・");

                size_t len2 = sep2 ? (size_t)(sep2 - rest) : strlen(rest);
                if (len2 > 255) len2 = 255;
                memcpy(secondary, rest, len2);
                secondary[len2] = '\0';
            } else {
                strncpy(primary, ant_str, 255);
                primary[255] = '\0';
            }

            int ant_entry_idx = -1;

            // keb
            for (uint32_t m = 0; m < dict->entry_count && ant_entry_idx == -1; ++m) {
                const entry_bin *ae = kotoba_entry(dict, m);

                for (uint32_t n = 0; n < ae->k_elements_count; ++n) {
                    const k_ele_bin *k = kotoba_k_ele(dict, ae, n);
                    kotoba_str ks = kotoba_keb(dict, k);

                    char buf[256];
                    uint32_t len = ks.len > 255 ? 255 : ks.len;
                    memcpy(buf, ks.ptr, len);
                    buf[len] = '\0';

                    if (strcmp(buf, primary) == 0) {
                        ant_entry_idx = m;
                        break;
                    }
                }
            }

            // reb (global FIX)
            if (ant_entry_idx == -1 && secondary[0]) {
                for (uint32_t m = 0; m < dict->entry_count && ant_entry_idx == -1; ++m) {
                    const entry_bin *ae = kotoba_entry(dict, m);

                    for (uint32_t n = 0; n < ae->r_elements_count; ++n) {
                        const r_ele_bin *r = kotoba_r_ele(dict, ae, n);
                        kotoba_str rs = kotoba_reb(dict, r);

                        char buf[256];
                        uint32_t len = rs.len > 255 ? 255 : rs.len;
                        memcpy(buf, rs.ptr, len);
                        buf[len] = '\0';

                        if (strcmp(buf, secondary) == 0) {
                            ant_entry_idx = m;
                            break;
                        }
                    }
                }
            }

            // fallback
            if (ant_entry_idx == -1) {
                for (uint32_t m = 0; m < dict->entry_count && ant_entry_idx == -1; ++m) {
                    const entry_bin *ae = kotoba_entry(dict, m);

                    for (uint32_t n = 0; n < ae->r_elements_count; ++n) {
                        const r_ele_bin *r = kotoba_r_ele(dict, ae, n);
                        kotoba_str rs = kotoba_reb(dict, r);

                        char buf[256];
                        uint32_t len = rs.len > 255 ? 255 : rs.len;
                        memcpy(buf, rs.ptr, len);
                        buf[len] = '\0';

                        if (strcmp(buf, primary) == 0) {
                            ant_entry_idx = m;
                            break;
                        }
                    }
                }
            }

            if (ant_entry_idx != -1) {
                long off = kotoba_sense(dict, kotoba_entry(dict, entry_id), sense_id)->ant_off
                           + i * sizeof(uint32_t);

                fseek(w->bin, off, SEEK_SET);
                write_u32(w->bin, (uint32_t)ant_entry_idx);
            } else {
                fprintf(stderr,
                        "Error: no se encontró ant '%s' para entry_id %u sense_id %u\n",
                        ant_str, entry_id, sense_id);
            }
        }
    }

    fclose(f);
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

int kotoba_writer_open_patch(kotoba_writer *w,
                       const char *bin_path,
                       const char *idx_path,
                       const kotoba_dict *dict)
{
    memset(w, 0, sizeof(*w));

    w->bin = fopen(bin_path, "r+b");
    if (!w->bin)
        return 0;

    w->idx = fopen(idx_path, "r+b");
    if (!w->idx)
        return 0;

    w->entry_count = dict->entry_count;

    return 1;

}