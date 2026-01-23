#include <stdio.h>
#include <stdlib.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../kotoba-core/include/types.h"
#include "../../kotoba-core/include/writer.h"
#include "../../kotoba-core/include/loader.h"
#include "../../kotoba-core/include/viewer.h"
#include "../../kotoba-core/include/kana.h"

#include <sqlite3.h>

const char *dict_path = "dict.kotoba";
const char *idx_path = "dict.kotoba.idx";

#define ROUTE_JMDICT_IDX 0
#define MAX_ENTRIES 209000
char *routes[] = {
    "../assets/JMdict",
};
int routes_count = sizeof(routes) / sizeof(routes[0]);
void parse_jmdict(xmlNodePtr root, kotoba_writer *writer)
{
    writer->entry_count = 0;
    entry e;
    for (xmlNodePtr cur_node = root->children; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (xmlStrcmp(cur_node->name, (const xmlChar *)"entry") == 0)
            {
                memset(&e, 0, sizeof(entry));
                for (xmlNodePtr child_node = cur_node->children; child_node; child_node = child_node->next)
                {
                    if (child_node->type == XML_ELEMENT_NODE)
                    {
                        xmlChar *content = xmlNodeGetContent(child_node);
                        if (xmlStrcmp(child_node->name, (const xmlChar *)"ent_seq") == 0)
                        {
                            e.ent_seq = atoi((const char *)content);
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"k_ele") == 0)
                        {
                            if (e.k_elements_count < MAX_K_ELEMENTS)
                            {
                                k_ele *k = &e.k_elements[e.k_elements_count];
                                memset(k, 0, sizeof(k_ele));
                                for (xmlNodePtr k_child = child_node->children; k_child; k_child = k_child->next)
                                {
                                    if (k_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *k_content = xmlNodeGetContent(k_child);
                                        if (xmlStrcmp(k_child->name, (const xmlChar *)"keb") == 0)
                                        {
                                            strncpy(k->keb, (const char *)k_content, MAX_KEB_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_inf") == 0)
                                        {
                                            if (k->ke_inf_count < MAX_KE_INF)
                                                strncpy(k->ke_inf[k->ke_inf_count++], (const char *)k_content, MAX_KE_INF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_KE_INF (%d)\n", e.ent_seq, MAX_KE_INF);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_pri") == 0)
                                        {
                                            if (k->ke_pri_count < MAX_KE_PRI)
                                                strncpy(k->ke_pri[k->ke_pri_count++], (const char *)k_content, MAX_KE_PRI_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_KE_PRI (%d)\n", e.ent_seq, MAX_KE_PRI);
                                        }
                                        xmlFree(k_content);
                                    }
                                }
                                e.k_elements_count++;
                            }
                            else
                            {
                                printf("[WARN] ent_seq=%u: se excedió MAX_K_ELEMENTS (%d)\n", e.ent_seq, MAX_K_ELEMENTS);
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"r_ele") == 0)
                        {
                            if (e.r_elements_count < MAX_R_ELEMENTS)
                            {
                                r_ele *r = &e.r_elements[e.r_elements_count];
                                memset(r, 0, sizeof(r_ele));
                                for (xmlNodePtr r_child = child_node->children; r_child; r_child = r_child->next)
                                {
                                    if (r_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *r_content = xmlNodeGetContent(r_child);
                                        if (xmlStrcmp(r_child->name, (const xmlChar *)"reb") == 0)
                                        {
                                            strncpy(r->reb, (const char *)r_content, MAX_REB_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_nokanji") == 0)
                                        {
                                            r->re_nokanji = true;
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_restr") == 0)
                                        {
                                            if (r->re_restr_count < MAX_RE_RESTR)
                                                strncpy(r->re_restr[r->re_restr_count++], (const char *)r_content, MAX_RE_RESTR_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_RE_RESTR (%d)\n", e.ent_seq, MAX_RE_RESTR);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_inf") == 0)
                                        {
                                            if (r->re_inf_count < MAX_RE_INF)
                                                strncpy(r->re_inf[r->re_inf_count++], (const char *)r_content, MAX_RE_INF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_RE_INF (%d)\n", e.ent_seq, MAX_RE_INF);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_pri") == 0)
                                        {
                                            if (r->re_pri_count < MAX_RE_PRI)
                                                strncpy(r->re_pri[r->re_pri_count++], (const char *)r_content, MAX_RE_PRI_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_RE_PRI (%d)\n", e.ent_seq, MAX_RE_PRI);
                                        }
                                        xmlFree(r_content);
                                    }
                                }
                                e.r_elements_count++;
                            }
                            else
                            {
                                printf("[WARN] ent_seq=%u: se excedió MAX_R_ELEMENTS (%d)\n", e.ent_seq, MAX_R_ELEMENTS);
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"sense") == 0)
                        {
                            if (e.senses_count < MAX_SENSES)
                            {
                                sense *s = &e.senses[e.senses_count];
                                memset(s, 0, sizeof(sense));
                                s->lang = KOTOBA_LANG_EN; // default
                                for (xmlNodePtr s_child = child_node->children; s_child; s_child = s_child->next)
                                {
                                    if (s_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *s_content = xmlNodeGetContent(s_child);
                                        if (xmlStrcmp(s_child->name, (const xmlChar *)"stagk") == 0)
                                        {
                                            if (s->stagk_count < MAX_STAGK)
                                                strncpy(s->stagk[s->stagk_count++], (const char *)s_content, MAX_STAGK_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_STAGK (%d)\n", e.ent_seq, MAX_STAGK);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"stagr") == 0)
                                        {
                                            if (s->stagr_count < MAX_STAGR)
                                                strncpy(s->stagr[s->stagr_count++], (const char *)s_content, MAX_STAGR_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_STAGR (%d)\n", e.ent_seq, MAX_STAGR);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"pos") == 0)
                                        {
                                            if (s->pos_count < MAX_POS)
                                                strncpy(s->pos[s->pos_count++], (const char *)s_content, MAX_POS_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_POS (%d)\n", e.ent_seq, MAX_POS);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"xref") == 0)
                                        {
                                            if (s->xref_count < MAX_XREF)
                                                strncpy(s->xref[s->xref_count++], (const char *)s_content, MAX_XREF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_XREF (%d)\n", e.ent_seq, MAX_XREF);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"ant") == 0)
                                        {
                                            if (s->ant_count < MAX_ANT)
                                                strncpy(s->ant[s->ant_count++], (const char *)s_content, MAX_ANT_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_ANT (%d)\n", e.ent_seq, MAX_ANT);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"field") == 0)
                                        {
                                            if (s->field_count < MAX_FIELD)
                                                strncpy(s->field[s->field_count++], (const char *)s_content, MAX_FIELD_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_FIELD (%d)\n", e.ent_seq, MAX_FIELD);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"misc") == 0)
                                        {
                                            if (s->misc_count < MAX_MISC)
                                                strncpy(s->misc[s->misc_count++], (const char *)s_content, MAX_MISC_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_MISC (%d)\n", e.ent_seq, MAX_MISC);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"s_inf") == 0)
                                        {
                                            if (s->s_inf_count < MAX_S_INF)
                                                strncpy(s->s_inf[s->s_inf_count++], (const char *)s_content, MAX_S_INF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_S_INF (%d)\n", e.ent_seq, MAX_S_INF);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"lsource") == 0)
                                        {
                                            if (s->lsource_count < MAX_LSOURCE)
                                                strncpy(s->lsource[s->lsource_count++], (const char *)s_content, MAX_LSOURCE_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_LSOURCE (%d)\n", e.ent_seq, MAX_LSOURCE);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"dial") == 0)
                                        {
                                            if (s->dial_count < MAX_DIAL)
                                                strncpy(s->dial[s->dial_count++], (const char *)s_content, MAX_DIAL_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_DIAL (%d)\n", e.ent_seq, MAX_DIAL);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"gloss") == 0)
                                        {
                                            // Set lang if first gloss and lang attribute exists
                                            if (s->gloss_count == 0)
                                            {
                                                xmlChar *lang_attr = xmlGetProp(s_child, (const xmlChar *)"lang");
                                                if (lang_attr)
                                                {
                                                    if (xmlStrcmp(lang_attr, (const xmlChar *)"eng") == 0)
                                                        s->lang = KOTOBA_LANG_EN;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fra") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"fre") == 0)
                                                        s->lang = KOTOBA_LANG_FR;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"deu") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"ger") == 0)
                                                        s->lang = KOTOBA_LANG_DE;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"rus") == 0)
                                                        s->lang = KOTOBA_LANG_RU;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"spa") == 0)
                                                        s->lang = KOTOBA_LANG_ES;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"por") == 0)
                                                        s->lang = KOTOBA_LANG_PT;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ita") == 0)
                                                        s->lang = KOTOBA_LANG_IT;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"nld") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"dut") == 0)
                                                        s->lang = KOTOBA_LANG_NL;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"hun") == 0)
                                                        s->lang = KOTOBA_LANG_HU;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"swe") == 0)
                                                        s->lang = KOTOBA_LANG_SV;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ces") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"cze") == 0)
                                                        s->lang = KOTOBA_LANG_CS;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"pol") == 0)
                                                        s->lang = KOTOBA_LANG_PL;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ron") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"rum") == 0)
                                                        s->lang = KOTOBA_LANG_RO;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"heb") == 0)
                                                        s->lang = KOTOBA_LANG_HE;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ara") == 0)
                                                        s->lang = KOTOBA_LANG_AR;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"tur") == 0)
                                                        s->lang = KOTOBA_LANG_TR;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"tha") == 0)
                                                        s->lang = KOTOBA_LANG_TH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"vie") == 0)
                                                        s->lang = KOTOBA_LANG_VI;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ind") == 0)
                                                        s->lang = KOTOBA_LANG_ID;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"msa") == 0)
                                                        s->lang = KOTOBA_LANG_MS;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"kor") == 0)
                                                        s->lang = KOTOBA_LANG_KO;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"zho") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"cmn") == 0)
                                                        s->lang = KOTOBA_LANG_ZH_CN;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"yue") == 0)
                                                        s->lang = KOTOBA_LANG_ZH_TW;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"wuu") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"nan") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"hak") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fas") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"per") == 0)
                                                        s->lang = KOTOBA_LANG_FA;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"epo") == 0)
                                                        s->lang = KOTOBA_LANG_EO;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"slv") == 0)
                                                        s->lang = KOTOBA_LANG_SLV;
                                                    else
                                                        s->lang = KOTOBA_LANG_UNK;
                                                    xmlFree(lang_attr);
                                                }
                                            }
                                            if (s->gloss_count < MAX_GLOSS)
                                                strncpy(s->gloss[s->gloss_count++], (const char *)s_content, MAX_GLOSS_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_GLOSS (%d)\n", e.ent_seq, MAX_GLOSS);
                                        }
                                        xmlFree(s_content);
                                    }
                                }
                                e.senses_count++;
                            }
                            else
                            {
                                printf("[WARN] ent_seq=%u: se excedió MAX_SENSES (%d)\n", e.ent_seq, MAX_SENSES);
                            }
                        }
                        xmlFree(content);
                    }
                }
                // write_entry_with_index(output_filename, idx_filename, &e);
                kotoba_writer_write_entry(writer, &e);
            }
        }
    }
}

/* ================= main ================= */
// index_cli.c
#define _POSIX_C_SOURCE 200809L
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>

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
#endif

/* ---------- TSV reader ---------- */
static int read_tsv_pairs(const char *tsv, const char ***texts_out, uint32_t **ids_out, uint32_t **ids2_out, uint32_t **ids3_out, size_t *count_out, bool read_fourth_col)
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
    uint32_t *ids = NULL, *ids2 = NULL, *ids3 = NULL;
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
            ids2 = realloc(ids2, cap * sizeof(uint32_t));
            if (read_fourth_col)
                ids3 = realloc(ids3, cap * sizeof(uint32_t));
            if (!texts || !ids || !ids2 || (read_fourth_col && !ids3))
            {
                perror("realloc");
                return -1;
            }
        }
        ids[cnt] = (uint32_t)atoi(id_s);
        texts[cnt] = strdup(txt);
        ids2[cnt] = id2_s ? (uint32_t)atoi(id2_s) : 0;
        if (read_fourth_col)
            ids3[cnt] = id3_s ? (uint32_t)atoi(id3_s) : 0;
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

static void free_pairs(const char **texts, uint32_t *ids, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        free((void *)texts[i]);
    free(texts);
    free(ids);
}
typedef void (*gram_cb)(const uint8_t *p, size_t len, void *ud);

void utf8_1_2_grams_cb(const char *s, gram_cb cb, void *ud)
{
    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *prev = NULL;
    size_t prev_len = 0;
    while (*p)
    {
        size_t len = utf8_char_len(*p);
        /* unigram */
        cb(p, len, ud);
        /* bigram (if we have prev) */
        if (prev)
        {
            /* copy prev + curr into small buffer (max 8 bytes for 4+4) */
            uint8_t tmp[8];
            memcpy(tmp, prev, prev_len);
            memcpy(tmp + prev_len, p, len);
            cb(tmp, prev_len + len, ud);
        }
        prev = p;
        prev_len = len;
        p += len;
    }
}

void print_grams(const char *s)
{
    void cb(const uint8_t *p, size_t len, void *ud)
    {
        printf("gram: %.*s  hash: %08x\n", (int)len, p, fnv1a(p, len));
    }
    utf8_1_2_grams_cb(s, cb, NULL);
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

#define SEARCH_MAX_RESULTS 1024
#define SEARCH_MAX_QUERY_HASHES 128
struct SearchContext
{
    InvertedIndex *kanji_idx;
    InvertedIndex *reading_idx;
    InvertedIndex *gloss_idxs;
    bool is_gloss_active[KOTOBA_LANG_COUNT];
    uint32_t *results;
    uint32_t result_count;
    kotoba_dict *dict;
};

void free_search_context(struct SearchContext *ctx)
{
    if (ctx->kanji_idx)
    {
        index_unload(ctx->kanji_idx);
        free(ctx->kanji_idx);
    }
    if (ctx->reading_idx)
    {
        index_unload(ctx->reading_idx);
        free(ctx->reading_idx);
    }
    if (ctx->gloss_idxs)
    {
        // debug
        for (size_t i = 0; i < KOTOBA_LANG_COUNT; ++i)
        {
            if (ctx->gloss_idxs[i].hdr)
            {
                index_unload(&ctx->gloss_idxs[i]);
            }
        }
        free(ctx->gloss_idxs);
    }
}

void init_search_context(struct SearchContext *ctx, kotoba_dict *dict)
{
    ctx->kanji_idx = NULL;
    ctx->reading_idx = NULL;
    ctx->gloss_idxs = NULL;
    ctx->result_count = 0;
    ctx->results = malloc(SEARCH_MAX_RESULTS * sizeof(uint32_t));
    ctx->dict = dict;

    ctx->kanji_idx = malloc(sizeof(InvertedIndex));
    if (ctx->kanji_idx && index_load("kanjis.idx", ctx->kanji_idx) != 0)
    {
        free(ctx->kanji_idx);
        ctx->kanji_idx = NULL;
    }

    ctx->reading_idx = malloc(sizeof(InvertedIndex));
    if (ctx->reading_idx && index_load("readings.idx", ctx->reading_idx) != 0)
    {
        free(ctx->reading_idx);
        ctx->reading_idx = NULL;
    }

    ctx->gloss_idxs = calloc(KOTOBA_LANG_COUNT, sizeof(InvertedIndex));
    if (ctx->gloss_idxs)
    {
        if (ctx->gloss_idxs)
        {
            // Only open English gloss index for debugging
            if (index_load("gloss_en.idx", &ctx->gloss_idxs[KOTOBA_LANG_EN]) != 0)
            {
                // If loading fails, set header to NULL for safety
                ctx->gloss_idxs[KOTOBA_LANG_EN].hdr = NULL;
                ctx->is_gloss_active[KOTOBA_LANG_EN] = false;
            }
            else
            {
                ctx->is_gloss_active[KOTOBA_LANG_EN] = true;
            }

            if (index_load("gloss_es.idx", &ctx->gloss_idxs[KOTOBA_LANG_ES]) != 0)
            {
                ctx->gloss_idxs[KOTOBA_LANG_ES].hdr = NULL;
                ctx->is_gloss_active[KOTOBA_LANG_ES] = false;
            }
            else
            {
                ctx->is_gloss_active[KOTOBA_LANG_ES] = true;
            }
            // All other gloss_idxs remain uninitialized (NULL)
        }
    }
}

void reset_search_context(struct SearchContext *ctx)
{
    ctx->result_count = 0;
}

int has_kanji(const char *query)
{
    const uint8_t *p = (const uint8_t *)query;
    while (*p)
    {
        size_t len = utf8_char_len(*p);
        if (len == 3)
        { // Kanji characters are typically 3 bytes in UTF-8
            uint32_t codepoint = 0;
            if (len == 3)
            {
                codepoint = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            }
            if (codepoint >= 0x4E00 && codepoint <= 0x9FAF)
            {
                return 1; // Found a Kanji character
            }
        }
        p += len;
    }
    return 0; // No Kanji characters found
}

// Devuelve 0 si es exacta, MAX_SCORE si no hay substring, o diferencia de longitudes si contiene
#define MAX_SCORE 1000000

int score_substr(const char *haystack, const char *needle, size_t nlen, size_t hlen)
{
    if (!*needle)
        return 0;
    if (nlen > hlen)
        return MAX_SCORE;
    if (strcmp(haystack, needle) == 0)
        return 0;
    const char *pos = strstr(haystack, needle);
    if (!pos)
        return MAX_SCORE;
    return (int)(hlen - nlen);
}

void query_search(struct SearchContext *ctx, const char *query)
{
    reset_search_context(ctx);

    uint32_t hashes[SEARCH_MAX_QUERY_HASHES];
    size_t hcount = query_1_2_gram_hashes(query, hashes, SEARCH_MAX_QUERY_HASHES);
    if (hcount == 0)
    {
        fprintf(stderr, "no grams from query\n");
        return;
    }

    size_t total_results = 0;
    char mixed[256];
    mixed_to_hiragana(query, mixed, sizeof(mixed));
    uint32_t mixed_hashes[SEARCH_MAX_QUERY_HASHES];
    size_t mixed_hcount = query_1_2_gram_hashes(mixed, mixed_hashes, SEARCH_MAX_QUERY_HASHES);

    // Search Kanji index if query contains Kanji
    if (has_kanji(query) && ctx->kanji_idx)
    {
        size_t rcount = index_intersect_hashes(
            ctx->kanji_idx,
            mixed_hashes,
            mixed_hcount,
            ctx->results + total_results,
            SEARCH_MAX_RESULTS - total_results);
        total_results += rcount;
    }

    // Search Reading index
    if (ctx->reading_idx)
    {
        size_t rcount = index_intersect_hashes(
            ctx->reading_idx,
            mixed_hashes,
            mixed_hcount,
            ctx->results + total_results,
            SEARCH_MAX_RESULTS - total_results);
        total_results += rcount;
    }

    // Search active Gloss indexes (use plain query, not mixed)
    for (size_t lang = 0; lang < KOTOBA_LANG_COUNT; ++lang)
    {
        if (ctx->is_gloss_active[lang])
        {
            InvertedIndex *gloss_idx = &ctx->gloss_idxs[lang];
            size_t rcount = index_intersect_hashes(
                gloss_idx,
                hashes,
                hcount,
                ctx->results + total_results,
                SEARCH_MAX_RESULTS - total_results);
            total_results += rcount;
        }
    }

    ctx->result_count = total_results;
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
    FILE *f = fopen("./tsv/kanjis.tsv", "w");
    if (!f)
    {
        perror("fopen kanjis.tsv");
        return;
    }

    FILE *f2 = fopen("./tsv/readings.tsv", "w");
    if (!f2)
    {
        perror("fopen readings.tsv");
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
    for (size_t i = 0; i < KOTOBA_LANG_COUNT; ++i)
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
        entry_bin *e = kotoba_entry(d, i);

        for (uint32_t j = 0; j < e->k_elements_count; ++j)
        {
            const k_ele_bin *k = kotoba_k_ele(d, e, j);
            kotoba_str keb = kotoba_keb(d, k);

            fprintf(f, "%u\t%.*s\t%u\n", i, keb.len, keb.ptr, j);
        }

        for (uint32_t r = 0; r < e->r_elements_count; ++r)
        {
            const r_ele_bin *re = kotoba_r_ele(d, e, r);
            kotoba_str reb = kotoba_reb(d, re);

            fprintf(f2, "%u\t%.*s\t%u\n", i, reb.len, reb.ptr, r);
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

/* ---------- CLI ---------- */
int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Set Windows console to UTF-8 for input/output */
    system("chcp 65001");
#endif

    if (argc < 2)
    {
        fprintf(stderr,
                "Usage:  build <tsv> out.idx\n"
                "        search <index.idx> <query>\n");
        return 1;
    }

    /* ---------- build ---------- */
    if (strcmp(argv[1], "build") == 0)
    {
        if (argc != 5)
        {
            fprintf(stderr, "build <tsv> gloss out.idx\n"
                            "                            or build <tsv> nogloss out.idx\n");
            return 1;
        }

        const char *tsv = argv[2];
        const char *out = argv[4];
        const char *gloss_flag = argv[3];
        bool read_fourth_col = false;
        if (strcmp(gloss_flag, "gloss") == 0)
            read_fourth_col = true;
        else if (strcmp(gloss_flag, "nogloss") == 0)
            read_fourth_col = false;
        else
        {
            fprintf(stderr, "invalid gloss flag: %s\n", gloss_flag);
            return 1;
        }

        const char **texts;
        uint32_t *ids;
        uint32_t *ids2;
        uint32_t *ids3;
        size_t n;

        if (read_tsv_pairs(tsv, &texts, &ids, &ids2, &ids3, &n, read_fourth_col) != 0)
            return 1;
        printf("a");
        int rc;
        if (read_fourth_col)
            rc = index_build_from_pairs(out, texts, ids, ids2, ids3, n);
        else
            rc = index_build_from_pairs(out, texts, ids, ids2, NULL, n);
        free_pairs(texts, ids, n);
        free(ids2);
        if (read_fourth_col)
            free(ids3);
        return rc == 0 ? 0 : 1;
    }

    /* ---------- search ---------- */
    else if (strcmp(argv[1], "search") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "search <idx> <query> \n");
            return 1;
        }

        const char *query_file = argv[3];

        char *query = read_utf8_file(query_file);
        if (!query)
        {
            fprintf(stderr, "failed read query file\n");
            return 1;
        }

        print_grams(query);

        InvertedIndex idx;
        const char *invdx_path = argv[2];
        if (index_load(invdx_path, &idx) != 0)
        {
            fprintf(stderr, "failed to load index: %s\n", invdx_path);
            return 1;
        }

        uint32_t hashes[128];
        size_t hcount = query_1_2_gram_hashes(query, hashes, 128);
        if (hcount == 0)
        {
            fprintf(stderr, "no grams from query\n");
            index_unload(&idx);
            return 1;
        }



        PostingRef  results[1024];
        size_t rcount = index_intersect_postings(
            &idx,
            hashes,
            hcount,
            results,
            1024);

        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);

        printf("Results (%zu):\n", rcount);
        for (size_t i = 0; i < rcount; ++i)
        {
                printf(
            "doc_id=%u reading_id=%u reading_variant=%u\n",
            results[i].p->doc_id,
            results[i].p->meta1,
            results[i].p->meta2
        );
            print_entry(&d, results[i].p->doc_id);
        }

        index_unload(&idx);
        return 0;
    }

    /* ---------- unknown ---------- */
    else
    {
        fprintf(stderr, "unknown command\n");
        return 1;
    }
}
