#include "kotoba_writer.h"
#include <string.h>

/* =========================================================
 * Header global
 * ========================================================= */

int kotoba_write_header(FILE *fp, kotoba_bin_header *header)
{
    if (!fp) return 0;

    kotoba_bin_header hdr;
    if (header) {
        hdr = *header;
    } else {
        memset(&hdr, 0, sizeof(hdr));
        hdr.magic = KOTOBA_MAGIC;
        hdr.version = KOTOBA_VERSION;
    }

    long cur = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fseek(fp, cur, SEEK_SET);

    return 1;
}

/* =========================================================
 * Helpers internos
 * ========================================================= */

static inline void align4(FILE *fp)
{
    long pos = ftell(fp);
    int pad = (4 - (pos & 3)) & 3;
    while (pad--) fputc(0, fp);
}

static inline uint32_t
write_string(FILE *fp, const char *s, uint32_t base)
{
    uint32_t off = (uint32_t)(ftell(fp) - base);
    fwrite(s, 1, strlen(s) + 1, fp);
    return off;
}

static inline uint32_t
write_offsets(FILE *fp, const uint32_t *o, uint32_t n, uint32_t base)
{
    if (!n) return 0;
    align4(fp);
    uint32_t off = (uint32_t)(ftell(fp) - base);
    fwrite(o, sizeof(uint32_t), n, fp);
    return off;
}

static inline void
entry_bin_init(entry_bin *eb, const entry *e)
{
    memset(eb, 0, sizeof(*eb));
    eb->ent_seq  = e->ent_seq;
    eb->k_count  = e->k_elements_count;
    eb->r_count  = e->r_elements_count;
    eb->s_count  = e->senses_count;
    eb->priority = e->priority;
}

/* =========================================================
 * Escritura principal
 * ========================================================= */

uint32_t write_entry(FILE *fp, const entry *e)
{
    uint32_t start = (uint32_t)ftell(fp);

    entry_bin eb;
    entry_bin_init(&eb, e);
    fwrite(&eb, sizeof(eb), 1, fp);

    /* k_ele */
    eb.k_off = (uint32_t)(ftell(fp) - start);
    for (int i = 0; i < e->k_elements_count; i++) {
        k_ele_bin b = {0};
        b.keb_off = write_string(fp, e->k_elements[i].keb, start);
        fwrite(&b, sizeof(b), 1, fp);
    }

    /* r_ele */
    eb.r_off = (uint32_t)(ftell(fp) - start);
    for (int i = 0; i < e->r_elements_count; i++) {
        r_ele_bin b = {0};
        b.reb_off = write_string(fp, e->r_elements[i].reb, start);
        fwrite(&b, sizeof(b), 1, fp);
    }

    /* sense */
    eb.s_off = (uint32_t)(ftell(fp) - start);
    for (int i = 0; i < e->senses_count; i++) {
        sense_bin sb = {0};
        sb.lang = e->senses[i].lang;
        fwrite(&sb, sizeof(sb), 1, fp);
    }

    uint32_t end = (uint32_t)ftell(fp);
    eb.buffer_off  = sizeof(entry_bin);
    eb.buffer_size = end - start;
    eb.total_size  = eb.buffer_size;

    fseek(fp, start, SEEK_SET);
    fwrite(&eb, sizeof(eb), 1, fp);
    fseek(fp, 0, SEEK_END);

    return start;
}

uint32_t write_entry_with_index(FILE *fp_bin, FILE *fp_idx, const entry *e)
{
    uint32_t off = write_entry(fp_bin, e);
    entry_index idx = { off };
    fwrite(&idx, sizeof(idx), 1, fp_idx);
    return off;
}
