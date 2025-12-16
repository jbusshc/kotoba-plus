#include "kotoba_writer.h"
#include <string.h>

/* =========================================================
 * Headers
 * ========================================================= */

int kotoba_write_bin_header(FILE *fp, uint32_t entry_count)
{
    if (!fp) return 0;

    kotoba_bin_header h = {
        .magic = KOTOBA_MAGIC,
        .version = KOTOBA_VERSION,
        .type = KOTOBA_FILE_BIN,
        .entry_count = entry_count
    };

    fseek(fp, 0, SEEK_SET);
    fwrite(&h, sizeof(h), 1, fp);
    fseek(fp, 0, SEEK_END);
    return 1;
}

int kotoba_write_idx_header(FILE *fp, uint32_t entry_count)
{
    if (!fp) return 0;

    kotoba_idx_header h = {
        .magic = KOTOBA_MAGIC,
        .version = KOTOBA_IDX_VERSION,
        .type = KOTOBA_FILE_IDX,
        .entry_count = entry_count,
        .entry_stride = sizeof(entry_index)
    };

    fseek(fp, 0, SEEK_SET);
    fwrite(&h, sizeof(h), 1, fp);
    fseek(fp, 0, SEEK_END);
    return 1;
}


/* =========================================================
 * Helpers internos
 * ========================================================= */
static inline void align4(FILE *fp)
{
    long pos = ftell(fp);
    while (pos & 3) {
        fputc(0, fp);
        pos++;
    }
}

uint32_t write_entry(FILE *fp, const entry *e)
{
    uint32_t start = (uint32_t)ftell(fp);

    entry_bin eb;
    memset(&eb, 0, sizeof(eb));
    eb.ent_seq  = e->ent_seq;
    eb.k_count  = e->k_elements_count;
    eb.r_count  = e->r_elements_count;
    eb.s_count  = e->senses_count;
    eb.priority = e->priority;

    fwrite(&eb, sizeof(eb), 1, fp);

    /* ---- k_ele ---- */
    align4(fp);
    eb.k_off = (uint32_t)(ftell(fp) - start);

    for (int i = 0; i < e->k_elements_count; i++) {
        k_ele_bin b = {0};
        b.keb_off = (uint32_t)(ftell(fp) - start);
        fwrite(e->k_elements[i].keb,
               1, strlen(e->k_elements[i].keb) + 1, fp);
        align4(fp);
        fwrite(&b, sizeof(b), 1, fp);
    }

    /* ---- r_ele ---- */
    align4(fp);
    eb.r_off = (uint32_t)(ftell(fp) - start);

    for (int i = 0; i < e->r_elements_count; i++) {
        r_ele_bin b = {0};
        b.reb_off = (uint32_t)(ftell(fp) - start);
        fwrite(e->r_elements[i].reb,
               1, strlen(e->r_elements[i].reb) + 1, fp);
        align4(fp);
        fwrite(&b, sizeof(b), 1, fp);
    }

    /* ---- sense ---- */
    align4(fp);
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
    fseek(fp, end, SEEK_SET);

    return start; /* offset absoluto */
}


uint32_t write_entry_with_index(FILE *fp_bin,
                                FILE *fp_idx,
                                const entry *e)
{
    uint32_t off = write_entry(fp_bin, e);
    entry_index idx = { off };
    fwrite(&idx, sizeof(idx), 1, fp_idx);
    return off;
}
