#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "kotoba_types.h"

// Helper to write a length-prefixed string
static uint32_t write_lp_string(FILE *f, const char *str)
{
    uint8_t len = (uint8_t)strlen(str);
    fwrite(&len, 1, 1, f);
    fwrite(str, 1, len, f);
    return 1 + len;
}

// Write k_ele_bin array and return offset
static uint32_t write_k_ele_bin(FILE *f, k_ele *k_elements, int count, uint32_t *str_off, uint32_t *str_base)
{
    uint32_t start = (uint32_t)ftell(f);
    for (int i = 0; i < count; ++i)
    {
        k_ele_bin keb;
        keb.keb_off = *str_off;
        keb.keb_len = (uint32_t)strlen(k_elements[i].keb);
        *str_off += write_lp_string(f, k_elements[i].keb);

        keb.ke_inf_off = *str_off;
        keb.ke_inf_count = k_elements[i].ke_inf_count;
        for (int j = 0; j < k_elements[i].ke_inf_count; ++j)
            *str_off += write_lp_string(f, k_elements[i].ke_inf[j]);

        keb.ke_pri_off = *str_off;
        keb.ke_pri_count = k_elements[i].ke_pri_count;
        for (int j = 0; j < k_elements[i].ke_pri_count; ++j)
            *str_off += write_lp_string(f, k_elements[i].ke_pri[j]);

        fwrite(&keb, sizeof(keb), 1, f);
    }
    return start;
}

// Write r_ele_bin array and return offset
static uint32_t write_r_ele_bin(FILE *f, r_ele *r_elements, int count, uint32_t *str_off, uint32_t *str_base)
{
    uint32_t start = (uint32_t)ftell(f);
    for (int i = 0; i < count; ++i)
    {
        r_ele_bin reb;
        reb.reb_off = *str_off;
        reb.reb_len = (uint32_t)strlen(r_elements[i].reb);
        *str_off += write_lp_string(f, r_elements[i].reb);

        reb.re_restr_off = *str_off;
        reb.re_restr_count = r_elements[i].re_restr_count;
        for (int j = 0; j < r_elements[i].re_restr_count; ++j)
            *str_off += write_lp_string(f, r_elements[i].re_restr[j]);

        reb.re_inf_off = *str_off;
        reb.re_inf_count = r_elements[i].re_inf_count;
        for (int j = 0; j < r_elements[i].re_inf_count; ++j)
            *str_off += write_lp_string(f, r_elements[i].re_inf[j]);

        reb.re_pri_off = *str_off;
        reb.re_pri_count = r_elements[i].re_pri_count;
        for (int j = 0; j < r_elements[i].re_pri_count; ++j)
            *str_off += write_lp_string(f, r_elements[i].re_pri[j]);

        fwrite(&reb, sizeof(reb), 1, f);
    }
    return start;
}

// Write sense_bin array and return offset
static uint32_t write_sense_bin(FILE *f, sense *senses, int count, uint32_t *str_off, uint32_t *str_base)
{
    uint32_t start = (uint32_t)ftell(f);
    for (int i = 0; i < count; ++i)
    {
        sense_bin sb = {0};
        sb.stagk_off = *str_off;
        sb.stagk_count = senses[i].stagk_count;
        for (int j = 0; j < senses[i].stagk_count; ++j)
            *str_off += write_lp_string(f, senses[i].stagk[j]);

        sb.stagr_off = *str_off;
        sb.stagr_count = senses[i].stagr_count;
        for (int j = 0; j < senses[i].stagr_count; ++j)
            *str_off += write_lp_string(f, senses[i].stagr[j]);

        sb.pos_off = *str_off;
        sb.pos_count = senses[i].pos_count;
        for (int j = 0; j < senses[i].pos_count; ++j)
            *str_off += write_lp_string(f, senses[i].pos[j]);

        sb.xref_off = *str_off;
        sb.xref_count = senses[i].xref_count;
        for (int j = 0; j < senses[i].xref_count; ++j)
            *str_off += write_lp_string(f, senses[i].xref[j]);

        sb.ant_off = *str_off;
        sb.ant_count = senses[i].ant_count;
        for (int j = 0; j < senses[i].ant_count; ++j)
            *str_off += write_lp_string(f, senses[i].ant[j]);

        sb.field_off = *str_off;
        sb.field_count = senses[i].field_count;
        for (int j = 0; j < senses[i].field_count; ++j)
            *str_off += write_lp_string(f, senses[i].field[j]);

        sb.misc_off = *str_off;
        sb.misc_count = senses[i].misc_count;
        for (int j = 0; j < senses[i].misc_count; ++j)
            *str_off += write_lp_string(f, senses[i].misc[j]);

        sb.s_inf_off = *str_off;
        sb.s_inf_count = senses[i].s_inf_count;
        for (int j = 0; j < senses[i].s_inf_count; ++j)
            *str_off += write_lp_string(f, senses[i].s_inf[j]);

        sb.lsource_off = *str_off;
        sb.lsource_count = senses[i].lsource_count;
        for (int j = 0; j < senses[i].lsource_count; ++j)
            *str_off += write_lp_string(f, senses[i].lsource[j]);

        sb.dial_off = *str_off;
        sb.dial_count = senses[i].dial_count;
        for (int j = 0; j < senses[i].dial_count; ++j)
            *str_off += write_lp_string(f, senses[i].dial[j]);

        sb.gloss_off = *str_off;
        sb.gloss_count = senses[i].gloss_count;
        for (int j = 0; j < senses[i].gloss_count; ++j)
            *str_off += write_lp_string(f, senses[i].gloss[j]);

        sb.lang = (uint8_t)senses[i].lang;

        fwrite(&sb, sizeof(sb), 1, f);
    }
    return start;
}

// Write a single entry_bin and its sub-objects, return offset
static uint32_t write_entry_bin(FILE *f, entry *e, uint32_t *str_off, uint32_t *str_base)
{
    uint32_t entry_start = (uint32_t)ftell(f);

    entry_bin eb = {0};
    eb.ent_seq = e->ent_seq;
    eb.priority = (int16_t)e->priority;

    // Write k_ele_bin array
    eb.k_elements_off = (uint32_t)ftell(f) - *str_base;
    eb.k_elements_count = e->k_elements_count;
    write_k_ele_bin(f, e->k_elements, e->k_elements_count, str_off, str_base);

    // Write r_ele_bin array
    eb.r_elements_off = (uint32_t)ftell(f) - *str_base;
    eb.r_elements_count = e->r_elements_count;
    write_r_ele_bin(f, e->r_elements, e->r_elements_count, str_off, str_base);

    // Write sense_bin array
    eb.senses_off = (uint32_t)ftell(f) - *str_base;
    eb.senses_count = e->senses_count;
    write_sense_bin(f, e->senses, e->senses_count, str_off, str_base);

    // Go back and write entry_bin at entry_start
    fseek(f, entry_start, SEEK_SET);
    fwrite(&eb, sizeof(eb), 1, f);
    fseek(f, 0, SEEK_END);

    return entry_start - *str_base;
}

// Write kotoba_bin_header and all entries
int kotoba_write_bin(const char *filename, entry *entries, uint32_t entry_count)
{
    FILE *f = fopen(filename, "wb");
    if (!f)
        return -1;

    kotoba_bin_header hdr = {0};
    hdr.magic = KOTOBA_MAGIC;
    hdr.version = KOTOBA_VERSION;
    hdr.type = KOTOBA_FILE_BIN;
    hdr.entry_count = entry_count;

    fwrite(&hdr, sizeof(hdr), 1, f);

    uint32_t str_base = (uint32_t)ftell(f);
    uint32_t str_off = str_base;

    // Write all entries
    for (uint32_t i = 0; i < entry_count; ++i)
    {
        write_entry_bin(f, &entries[i], &str_off, &str_base);
    }

    fclose(f);
    return 0;
}