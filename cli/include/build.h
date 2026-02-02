#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../kotoba-core/include/types.h"
#include "../../kotoba-core/include/viewer.h"
#include "../../kotoba-core/include/loader.h"

/* strsep implementation for Windows */
char *strsep(char **stringp, const char *delim)
{
    char *start = *stringp;
    char *p;
    if (!start)
        return NULL;
    p = start + strcspn(start, delim);
    if (*p)
    {
        *p = '\0';
        *stringp = p + 1;
    }
    else
    {
        *stringp = NULL;
    }
    return start;
}

/* Simple getline replacement para Windows */
ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    if (!lineptr || !n || !stream)
        return -1;
    if (*lineptr == NULL || *n == 0)
    {
        *n = 4096;
        *lineptr = malloc(*n);
        if (!*lineptr)
            return -1;
    }
    size_t pos = 0;
    int c;
    while ((c = fgetc(stream)) != EOF)
    {
        if (pos + 1 >= *n)
        {
            *n *= 2;
            char *tmp = realloc(*lineptr, *n);
            if (!tmp)
                return -1;
            *lineptr = tmp;
        }
        (*lineptr)[pos++] = (char)c;
        if (c == '\n')
            break;
    }
    if (pos == 0 && c == EOF)
        return -1;
    (*lineptr)[pos] = '\0';
    return pos;
}

/* ---------- TSV reader ---------- */
static int read_tsv_pairs(const char *tsv, const char ***texts_out, uint32_t **ids_out, uint8_t **ids2_out, uint8_t **ids3_out, size_t *count_out, bool read_fourth_col)
{
    FILE *f = fopen(tsv, "r");
    if (!f)
    {
        perror("fopen tsv");
        return -1;
    }
    char *line = NULL;
    size_t lcap = 0;
    size_t cap = 0, cnt = 0;
    const char **texts = NULL;
    uint32_t *ids = NULL;
    uint8_t *ids2 = NULL, *ids3 = NULL;
    while (getline(&line, &lcap, f) != -1)
    {
        if (line[0] == '#' || line[0] == '\n')
            continue;
        char *p = line;
        char *id_s = strsep(&p, "\t\n");
        char *txt = strsep(&p, "\t\n");
        char *id2_s = strsep(&p, "\t\n");
        char *id3_s = NULL;
        if (read_fourth_col)
            id3_s = strsep(&p, "\t\n");
        if (!id_s || !txt)
            continue;
        if (cnt == cap)
        {
            cap = cap ? cap * 2 : 1024;
            texts = realloc(texts, cap * sizeof(char *));
            ids = realloc(ids, cap * sizeof(uint32_t));
            ids2 = realloc(ids2, cap * sizeof(uint8_t));
            if (read_fourth_col)
                ids3 = realloc(ids3, cap * sizeof(uint8_t));
            if (!texts || !ids || !ids2 || (read_fourth_col && !ids3))
            {
                perror("realloc");
                return -1;
            }
        }
        ids[cnt] = (uint32_t)atoi(id_s);
        texts[cnt] = strdup(txt);
        ids2[cnt] = id2_s ? (uint8_t)atoi(id2_s) : 0;
        if (read_fourth_col)
            ids3[cnt] = id3_s ? (uint8_t)atoi(id3_s) : 0;
        cnt++;
    }
    free(line);
    fclose(f);
    *texts_out = texts;
    *ids_out = ids;
    *ids2_out = ids2;
    if (read_fourth_col)
        *ids3_out = ids3;
    if (count_out)
        *count_out = cnt;
    return 0;
}

static void free_pairs(const char **texts, uint32_t *ids, uint8_t *ids2, uint8_t *ids3, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        free((void *)texts[i]);
    free(texts);
    free(ids);
    free(ids2);
    free(ids3);
}

static char *read_utf8_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror("fopen");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    if (fread(buf, 1, sz, f) != (size_t)sz)
    {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);

    buf[sz] = '\0';

    // trim newline / carriage return
    while (sz > 0 && (buf[sz - 1] == '\n' || buf[sz - 1] == '\r'))
        buf[--sz] = '\0';

    return buf;
}


void print_entry(const kotoba_dict *d, uint32_t i)
{
    const entry_bin *e = kotoba_entry(d, i);
    if (!e)
        return;

    printf("Entry %u\n", i);
    printf("  ent_seq = %d\n", e->ent_seq);
    /* -------------------------
     * Kanji
     * ------------------------- */

    printf("  k_elements_count = %u\n", e->k_elements_count);

    for (uint32_t i = 0; i < e->k_elements_count; ++i)
    {
        const k_ele_bin *k = kotoba_k_ele(d, e, i);
        kotoba_str keb = kotoba_keb(d, k);

        printf("  kanji[%u]: %.*s\n",
               i, keb.len, keb.ptr);
    }

    printf("  r_elements_count = %u\n", e->r_elements_count);
    /* -------------------------
     * Readings
     * ------------------------- */
    for (uint32_t i = 0; i < e->r_elements_count; ++i)
    {
        const r_ele_bin *r = kotoba_r_ele(d, e, i);
        printf("  post kotoba_r_ele\n");
        kotoba_str reb = kotoba_reb(d, r);

        printf("  reading[%u]: %.*s\n",
               i, reb.len, reb.ptr);
    }

    printf("  senses_count = %u\n", e->senses_count);
    /* -------------------------
     * Senses
     * ------------------------- */
    for (uint32_t si = 0; si < e->senses_count; ++si)
    {
        const sense_bin *s = kotoba_sense(d, e, si);

        printf("  sense[%u] (lang=%u)\n", si, s->lang);

        /* POS */
        for (uint32_t i = 0; i < s->pos_count; ++i)
        {
            kotoba_str pos = kotoba_pos(d, s, i);
            printf("    pos: %.*s\n", pos.len, pos.ptr);
        }

        /* Gloss */
        for (uint32_t i = 0; i < s->gloss_count; ++i)
        {
            kotoba_str g = kotoba_gloss(d, s, i);
            printf("    gloss: %.*s\n", g.len, g.ptr);
        }

        /* Misc (ejemplo) */
        for (uint32_t i = 0; i < s->misc_count; ++i)
        {
            kotoba_str m = kotoba_misc(d, s, i);
            printf("    misc: %.*s\n", m.len, m.ptr);
        }

        for (uint32_t i = 0; i < s->stagk_count; ++i)
        {
            kotoba_str stg = kotoba_stagk(d, s, i);
            printf("    stagk: %.*s\n", stg.len, stg.ptr);
        }

        for (uint32_t i = 0; i < s->stagr_count; ++i)
        {
            kotoba_str stg = kotoba_stagr(d, s, i);
            printf("    stagr: %.*s\n", stg.len, stg.ptr);
        }

        for (uint32_t i = 0; i < s->xref_count; ++i)
        {
            kotoba_str xref = kotoba_xref(d, s, i);
            printf("    xref: %.*s\n", xref.len, xref.ptr);
        }

        for (uint32_t i = 0; i < s->ant_count; ++i)
        {
            kotoba_str ant = kotoba_ant(d, s, i);
            printf("    ant: %.*s\n", ant.len, ant.ptr);
        }

        for (uint32_t i = 0; i < s->field_count; ++i)
        {
            kotoba_str field = kotoba_field(d, s, i);
            printf("    field: %.*s\n", field.len, field.ptr);
        }

        for (uint32_t i = 0; i < s->s_inf_count; ++i)
        {
            kotoba_str s_inf = kotoba_s_inf(d, s, i);
            printf("    s_inf: %.*s\n", s_inf.len, s_inf.ptr);
        }

        for (uint32_t i = 0; i < s->lsource_count; ++i)
        {
            kotoba_str lsource = kotoba_lsource(d, s, i);
            printf("    lsource: %.*s\n", lsource.len, lsource.ptr);
        }

        for (uint32_t i = 0; i < s->dial_count; ++i)
        {
            kotoba_str dial = kotoba_dial(d, s, i);
            printf("    dial: %.*s\n", dial.len, dial.ptr);
        }
    }
}

void write_all_tsv(const kotoba_dict *d)
{
    FILE *f = fopen("./tsv/jp.tsv", "w");
    if (!f)
    {
        perror("fopen kanjis.tsv");
        return;
    }

    FILE *lang_f[KOTOBA_LANG_COUNT] = {0};
    // Correspondence with enum kotoba_lang:
    // KOTOBA_LANG_EN = 0,
    // KOTOBA_LANG_FR,
    // KOTOBA_LANG_DE,
    // KOTOBA_LANG_RU,
    // KOTOBA_LANG_ES,
    // KOTOBA_LANG_PT,
    // KOTOBA_LANG_IT,
    // KOTOBA_LANG_NL,
    // KOTOBA_LANG_HU,
    // KOTOBA_LANG_SV,
    // KOTOBA_LANG_CS,
    // KOTOBA_LANG_PL,
    // KOTOBA_LANG_RO,
    // KOTOBA_LANG_HE,
    // KOTOBA_LANG_AR,
    // KOTOBA_LANG_TR,
    // KOTOBA_LANG_TH,
    // KOTOBA_LANG_VI,
    // KOTOBA_LANG_ID,
    // KOTOBA_LANG_MS,
    // KOTOBA_LANG_KO,
    // KOTOBA_LANG_ZH,
    // KOTOBA_LANG_ZH_CN,
    // KOTOBA_LANG_ZH_TW,
    // KOTOBA_LANG_FA,
    // KOTOBA_LANG_EO,
    // KOTOBA_LANG_SLV,

    const char *lang_fnames[KOTOBA_LANG_COUNT] = {
        "gloss_en.tsv",    // KOTOBA_LANG_EN
        "gloss_fr.tsv",    // KOTOBA_LANG_FR
        "gloss_de.tsv",    // KOTOBA_LANG_DE
        "gloss_ru.tsv",    // KOTOBA_LANG_RU
        "gloss_es.tsv",    // KOTOBA_LANG_ES
        "gloss_pt.tsv",    // KOTOBA_LANG_PT
        "gloss_it.tsv",    // KOTOBA_LANG_IT
        "gloss_nl.tsv",    // KOTOBA_LANG_NL
        "gloss_hu.tsv",    // KOTOBA_LANG_HU
        "gloss_sv.tsv",    // KOTOBA_LANG_SV
        "gloss_cs.tsv",    // KOTOBA_LANG_CS
        "gloss_pl.tsv",    // KOTOBA_LANG_PL
        "gloss_ro.tsv",    // KOTOBA_LANG_RO
        "gloss_he.tsv",    // KOTOBA_LANG_HE
        "gloss_ar.tsv",    // KOTOBA_LANG_AR
        "gloss_tr.tsv",    // KOTOBA_LANG_TR
        "gloss_th.tsv",    // KOTOBA_LANG_TH
        "gloss_vi.tsv",    // KOTOBA_LANG_VI
        "gloss_id.tsv",    // KOTOBA_LANG_ID
        "gloss_ms.tsv",    // KOTOBA_LANG_MS
        "gloss_ko.tsv",    // KOTOBA_LANG_KO
        "gloss_zh.tsv",    // KOTOBA_LANG_ZH
        "gloss_zh_cn.tsv", // KOTOBA_LANG_ZH_CN
        "gloss_zh_tw.tsv", // KOTOBA_LANG_ZH_TW
        "gloss_fa.tsv",    // KOTOBA_LANG_FA
        "gloss_eo.tsv",    // KOTOBA_LANG_EO
        "gloss_slv.tsv"    // KOTOBA_LANG_SLV
    };

    // Open all gloss language files at once
    char tsv_dir[64] = "./tsv/";
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        char path[256];
        snprintf(path, sizeof(path), "%s%s", tsv_dir, lang_fnames[i]);
        lang_f[i] = fopen(path, "w");
        if (!lang_f[i])
        {
            fprintf(stderr, "Failed to open %s for writing\n", path);
        }
    }

    for (uint32_t i = 0; i < d->entry_count; ++i)
    {
        const entry_bin *e = kotoba_entry(d, i);

        for (uint32_t j = 0; j < e->k_elements_count; ++j)
        {
            const k_ele_bin *k = kotoba_k_ele(d, e, j);
            kotoba_str keb = kotoba_keb(d, k);

            fprintf(f, "%u\t%.*s\t%u\t%u\n", i, keb.len, keb.ptr, j, TYPE_KANJI);
        }

        for (uint32_t r = 0; r < e->r_elements_count; ++r)
        {
            const r_ele_bin *re = kotoba_r_ele(d, e, r);
            kotoba_str reb = kotoba_reb(d, re);

            fprintf(f, "%u\t%.*s\t%u\t%u\n", i, reb.len, reb.ptr, r, TYPE_READING);
        }

        for (uint32_t s = 0; s < e->senses_count; ++s)
        {
            const sense_bin *se = kotoba_sense(d, e, s);

            for (uint32_t g = 0; g < se->gloss_count; ++g)
            {
                kotoba_str gloss = kotoba_gloss(d, se, g);
                uint32_t lang = se->lang;

                if (lang < KOTOBA_LANG_COUNT)
                {

                    fprintf(lang_f[lang], "%u\t%.*s\t%u\t%u\n", i, gloss.len, gloss.ptr, s, g);
                }
            }
        }
    }
}