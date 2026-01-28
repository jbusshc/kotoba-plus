#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../kotoba-core/include/types.h"
#include "../../kotoba-core/include/writer.h"
#include "../../kotoba-core/include/loader.h"
#include "../../kotoba-core/include/viewer.h"
#include "../../kotoba-core/include/kana.h"
#include "../../kotoba-core/include/index.h"


const char *dict_path = "dict.kotoba";
const char *idx_path = "dict.kotoba.idx";


#define _POSIX_C_SOURCE 200809L
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

void print_grams(const char *s)
{
    void cb(const uint8_t *p, size_t len, void *ud)
    {
        printf("gram: %.*s  hash: %08x\n", (int)len, p, fnv1a(p, len));
    }
    utf8_grams_cb(s, cb, NULL);
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

typedef struct
{
    const char *s;
    uint8_t len;
    char first;
    uint8_t _pad[6]; // pad to 16 bytes (8-byte aligned)
} query_t;

enum InputTypeFlag
{
    INPUT_TYPE_NONE = 0,
    INPUT_TYPE_KANJI = 1,
    INPUT_TYPE_KANA = 2,
    INPUT_TYPE_ROMAJI = 4
};

int get_input_type(const char *query)
{
    if (!query || *query == '\0')
        return INPUT_TYPE_NONE;
    int type = INPUT_TYPE_ROMAJI;
    const uint8_t *p = (const uint8_t *)query;
    while (*p)
    {
        int len = utf8_char_len(*p);
        uint32_t codepoint = 0;
        if (len == 3)
        {
            codepoint = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            if (codepoint >= 0x4E00 && codepoint <= 0x9FAF)
            {
                type = type | INPUT_TYPE_KANJI;
            }
            else if ((codepoint >= 0x3040 && codepoint <= 0x309F) || // Hiragana
                     (codepoint >= 0x30A0 && codepoint <= 0x30FF))
            { // Katakana
                type = type | INPUT_TYPE_KANA;
            }
        }
        p += len;
    }
    return type;
}

static inline int
word_contains_q(const char *word, int wlen,
                const query_t *q)
{
    if (q->len > wlen)
        return 0;

    const char *end = word + wlen - q->len;

    if (q->len == 1)
    {
        for (const char *p = word; p <= end; ++p)
            if (*p == q->first)
                return 1;
        return 0;
    }

    for (const char *p = word; p <= end; ++p)
    {
        if (*p == q->first &&
            memcmp(p + 1, q->s + 1, q->len - 1) == 0)
            return 1;
    }
    return 0;
}

typedef struct
{
    uint32_t results_idx; // index in results buffer
    uint16_t score;       // lower is better
    uint16_t type;        // 0=kanji,1=reading,2+=gloss lang index

} SearchResultMeta;

static inline void
sort_scores(SearchResultMeta *a, int n)
{
    for (int i = 1; i < n; ++i)
    {
        SearchResultMeta key = a[i];
        int j = i - 1;

        /* menor score = mejor */
        while (j >= 0 && a[j].score > key.score)
        {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = key;
    }
}

struct SearchContext
{
    bool is_gloss_active[KOTOBA_LANG_COUNT]; // 28 bytes
    uint8_t _pad0[4];                        // pad to 32 bytes (8-byte aligned)
    query_t query;
    kotoba_dict *dict;
    InvertedIndex *kanji_idx;
    InvertedIndex *reading_idx;
    InvertedIndex **gloss_idxs;
    PostingRef *results_buffer;
    uint32_t *results_doc_ids;
    SearchResultMeta *results;
    uint32_t results_left;
    uint32_t results_processed;
    uint32_t page_size;
    uint32_t last_page;
};

#define DEFAULT_PAGE_SIZE 10

void init_search_context(struct SearchContext *ctx,
                         bool *glosses_active,
                         kotoba_dict *dict,
                         uint32_t page_size)
{
    memset(ctx, 0, sizeof(struct SearchContext));
    ctx->dict = dict;
    ctx->page_size = page_size > 0 ? page_size : DEFAULT_PAGE_SIZE;
    ctx->results_left = 0;
    ctx->results_processed = 0;

    if (glosses_active)
    {
        for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
        {
            ctx->is_gloss_active[i] = glosses_active[i];
        }
    }

    ctx->kanji_idx = malloc(sizeof(InvertedIndex));
    ctx->reading_idx = malloc(sizeof(InvertedIndex));
    ctx->gloss_idxs = calloc(KOTOBA_LANG_COUNT, sizeof(InvertedIndex *));
    if (!ctx->kanji_idx || !ctx->reading_idx || !ctx->gloss_idxs) {
        fprintf(stderr, "Memory allocation failed in init_search_context\n");
        exit(1);
    }

    if (index_load("kanjis.idx", ctx->kanji_idx) != 0) {
        fprintf(stderr, "Failed to load kanjis.idx\n");
        exit(1);
    }
    if (index_load("readings.idx", ctx->reading_idx) != 0) {
        fprintf(stderr, "Failed to load readings.idx\n");
        exit(1);
    }

    // Only allocate and load gloss indexes for active languages
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        if (ctx->is_gloss_active[i])
        {
            ctx->gloss_idxs[i] = malloc(sizeof(InvertedIndex));
            if (!ctx->gloss_idxs[i]) {
                fprintf(stderr, "Memory allocation failed for gloss_idxs[%d]\n", i);
                exit(1);
            }
            char fname[64];
            // You may want to use a mapping for filenames instead of this switch
            switch (i) {
                case KOTOBA_LANG_EN: strcpy(fname, "gloss_en.idx"); break;
                case KOTOBA_LANG_ES: strcpy(fname, "gloss_es.idx"); break;
                // Add more cases as needed for other languages
                default: snprintf(fname, sizeof(fname), "gloss_%d.idx", i); break;
            }
            if (index_load(fname, ctx->gloss_idxs[i]) != 0) {
                fprintf(stderr, "Failed to load %s\n", fname);
                free(ctx->gloss_idxs[i]);
                ctx->gloss_idxs[i] = NULL;
            }
        }
        else
        {
            ctx->gloss_idxs[i] = NULL;
        }
    }

    ctx->results_doc_ids = malloc(sizeof(uint32_t) * SEARCH_MAX_RESULTS);
    ctx->results = malloc(sizeof(SearchResultMeta) * SEARCH_MAX_RESULTS);
    ctx->results_buffer = malloc(sizeof(PostingRef) * SEARCH_MAX_RESULTS);

    if (!ctx->results_doc_ids || !ctx->results || !ctx->results_buffer) {
        fprintf(stderr, "Memory allocation failed for results buffers\n");
        exit(1);
    }
}

void free_search_context(struct SearchContext *ctx)
{
    if (ctx->kanji_idx)
    {
        index_unload(ctx->kanji_idx);
    }
    if (ctx->reading_idx)
    {
        index_unload(ctx->reading_idx);
    }
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        if (ctx->gloss_idxs[i])
        {
            index_unload(ctx->gloss_idxs[i]);
            ctx->gloss_idxs[i] = NULL;
        }
    }
    free(ctx->results_doc_ids);
    free(ctx->results);
    free(ctx->results_buffer);

    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        if (ctx->gloss_idxs[i])
        {
            free(ctx->gloss_idxs[i]);
        }
    }
    free(ctx->gloss_idxs);
}

void reset_search_context(struct SearchContext *ctx)
{
    ctx->results_left = 0;
    ctx->results_processed = 0;
    ctx->last_page = 0;
}

void query_results(struct SearchContext *ctx, const char *query)
{
    reset_search_context(ctx);
    query_t q;
    q.s = query;
    q.len = (uint8_t)strlen(query);
    q.first = query[0];
    uint32_t hashes[SEARCH_MAX_QUERY_HASHES];
    int hcount = query_gram_hashes(query, hashes, SEARCH_MAX_QUERY_HASHES);
    if (hcount == 0)
    {
        fprintf(stderr, "no grams from query\n");
        return;
    }
    char mixed[256];
    mixed_to_hiragana(query, mixed, sizeof(mixed));

    query_t mixed_q;
    mixed_q.s = mixed;
    mixed_q.len = (uint8_t)strlen(mixed);
    mixed_q.first = mixed[0];
    uint32_t mixed_hashes[SEARCH_MAX_QUERY_HASHES];
    int mixed_hcount = query_gram_hashes(mixed, mixed_hashes, SEARCH_MAX_QUERY_HASHES);
    InvertedIndex *kanji_idx = ctx->kanji_idx;
    InvertedIndex *reading_idx = ctx->reading_idx;
    InvertedIndex **gloss_idxs = ctx->gloss_idxs;
    kotoba_dict *dict = ctx->dict;
    PostingRef *results_buffer = ctx->results_buffer;
    SearchResultMeta *results_meta = ctx->results; 
    int total_results = 0;
    int input_type = get_input_type(query);
    // Search Kanji index if query contains Kanji

    if (input_type & INPUT_TYPE_KANJI && kanji_idx)
    {
        int rcount = index_intersect_postings(
            kanji_idx,
            mixed_hashes,
            mixed_hcount,
            results_buffer,
            SEARCH_MAX_RESULTS);

        int discarded = 0;
        // getting scores
        for (int i = 0; i < rcount; ++i)
        {
            const entry_bin *e = kotoba_entry(dict, results_buffer[i].p->doc_id);
            int k_elem_id = results_buffer[i].p->meta1;
            const k_ele_bin *k_ele = kotoba_k_ele(dict, e, k_elem_id);
            kotoba_str keb = kotoba_keb(dict, k_ele);

            if (mixed_q.len > keb.len)
            { // if query longer than keb, false positive
                discarded++;
                continue;
            }

            results_meta[i].results_idx = i;
            results_meta[i].score = (uint16_t)keb.len;
            results_meta[i].type = 0; // kanji
        }
        total_results += rcount - discarded;
    }

    else if (reading_idx)
    {
        printf("parameters: mixed='%s' (%d hashes), space left: %u\n", mixed, mixed_hcount, SEARCH_MAX_RESULTS - total_results);
        int rcount = index_intersect_postings(
            reading_idx,
            mixed_hashes,
            mixed_hcount,
            results_buffer + total_results,
            SEARCH_MAX_RESULTS - total_results);
        int base = total_results;

        int discarded = 0;
        // getting scores
        for (int i = 0; i < rcount; ++i)
        {
            const entry_bin *e = kotoba_entry(
                dict,
                results_buffer[base + i].p->doc_id);
            int r_elem_id = results_buffer[base + i].p->meta1;
            const r_ele_bin *r_ele = kotoba_r_ele(dict, e, r_elem_id);
            kotoba_str reb = kotoba_reb(dict, r_ele);

            if (mixed_q.len > reb.len)
            { // if query longer than reb, false positive
                discarded++;
                continue;
            }

            results_meta[base + i].results_idx = base + i;
            results_meta[base + i].score = (uint16_t)reb.len;
            results_meta[base + i].type = 1; // reading
        }
        total_results += rcount - discarded;
    }

    /*
    if (!(input_type &  (INPUT_TYPE_KANJI | INPUT_TYPE_KANA ) )) {
    // Search gloss indexes
        for (int lang = 0; lang < KOTOBA_LANG_COUNT; ++lang)
        {
            if (ctx->is_gloss_active[lang] && gloss_idxs[lang])
            {
                int rcount = index_intersect_postings(
                    gloss_idxs[lang],
                    hashes,
                    hcount,
                    &results_buffer[total_results],
                    SEARCH_MAX_RESULTS - total_results);
                int base = total_results;

                int discarded = 0;
                // getting scores
                for (int i = 0; i < rcount; ++i)
                {
                    const entry_bin *e = kotoba_entry(
                        dict,
                        results_buffer[base + i].p->doc_id);
                    int s_elem_id = results_buffer[base + i].p->meta1;
                    int gloss_id = results_buffer[base + i].p->meta2;
                    const sense_bin *s = kotoba_sense(dict, e, s_elem_id);
                    kotoba_str gloss = kotoba_gloss(dict, s, gloss_id);

                    if (q.len > gloss.len)
                    { // if query longer than gloss, false positive
                        discarded++;
                        continue;
                    }

                    results_meta[base + i].results_idx = base + i;
                    results_meta[base + i].score = (uint16_t)gloss.len;
                    results_meta[base + i].type = 2; // + lang; // gloss lang index
                }
                total_results += rcount - discarded;
            }
        } 
    }
    */

    printf("mixed query: '%s' (%d hashes)\n", mixed, mixed_hcount);
    printf("input type: %s%s%s\n",
           (input_type & INPUT_TYPE_KANJI) ? "KANJI " : "",
           (input_type & INPUT_TYPE_KANA) ? "KANA " : "",
           (input_type & INPUT_TYPE_ROMAJI) ? "ROMAJI " : "");

    // end
    ctx->query = q;
    ctx->results_left = total_results;
}

void query_next_page(struct SearchContext *ctx)
{
    kotoba_dict *dict = ctx->dict;
    PostingRef *results_buffer = ctx->results_buffer;
    SearchResultMeta *results_meta = ctx->results;
    uint32_t results_left = ctx->results_left;
    uint32_t page_size = ctx->page_size;
    uint32_t last_page = ctx->last_page;

    if (results_left == 0)
    {
        printf("No more results\n");
        return;
    }

    page_size = page_size < results_left ? page_size : results_left;

    SearchResultMeta buffer[page_size];
    int buffer_meta_pos[page_size];   // 🔥 POSICIÓN REAL EN results_meta
    int buf_size = 0;
    int worst_idx = 0;

    query_t q = ctx->query;
    char mixed[256];
    mixed_to_hiragana(q.s, mixed, sizeof(mixed));
    query_t mixed_q = { .s = mixed, .len = (uint8_t)strlen(mixed), .first = mixed[0] };

    for (uint32_t i = 0; i < results_left; ++i)
    {
    skip:
        int score = results_meta[i].score;

        // ✅ comparar contra BUFFER, no results_meta
        if (buf_size == page_size && score >= buffer[worst_idx].score)
            continue;

        // ---- filtro de string ----
        const entry_bin *e = kotoba_entry(
            dict,
            results_buffer[results_meta[i].results_idx].p->doc_id
        );

        int type = results_meta[i].type;
        int meta1 = results_buffer[results_meta[i].results_idx].p->meta1;
        int meta2 = results_buffer[results_meta[i].results_idx].p->meta2;

        kotoba_str str;
        query_t *query;
        if (type == 0)
        {
            const k_ele_bin *k_ele = kotoba_k_ele(dict, e, meta1);
            str = kotoba_keb(dict, k_ele);
            query = &mixed_q;
        }
        else if (type == 1)
        {
            const r_ele_bin *r_ele = kotoba_r_ele(dict, e, meta1);
            str = kotoba_reb(dict, r_ele);
            query = &mixed_q;
        }
        else
        {
            const sense_bin *s = kotoba_sense(dict, e, meta1);
            str = kotoba_gloss(dict, s, meta2);
            query = &q;
        }

        if (!word_contains_q(str.ptr, str.len, query))
        {
            if (i < results_left - 1)
            {
                SearchResultMeta tmp = results_meta[i];
                results_meta[i] = results_meta[results_left - 1];
                results_meta[results_left - 1] = tmp;
                --results_left;
                goto skip;
            }
            else
            {
                --results_left;
                continue;   // ✅ FIX
            }
        }

        // ---- top-K ----
        if (buf_size < page_size)
        {
            buffer[buf_size] = results_meta[i];
            buffer_meta_pos[buf_size] = i;

            if (buf_size == 0 || score > buffer[worst_idx].score)
                worst_idx = buf_size;

            ++buf_size;
        }
        else
        {
            buffer[worst_idx] = results_meta[i];
            buffer_meta_pos[worst_idx] = i;

            // recompute worst (score más alto)
            int ws = buffer[0].score;
            int wi = 0;
            for (int j = 1; j < buf_size; ++j)
            {
                if (buffer[j].score > ws)
                {
                    ws = buffer[j].score;
                    wi = j;
                }
            }
            worst_idx = wi;
        }
    }

    // ---- ordenar página ----
    sort_scores(buffer, buf_size);

    int offset = last_page * page_size;
    for (int i = 0; i < buf_size; ++i)
    {
        ctx->results_doc_ids[offset + i] =
            results_buffer[buffer[i].results_idx].p->doc_id;
    }

    // ---- borrar procesados (EN ORDEN INVERSO) ----
    for (int i = buf_size - 1; i >= 0; --i)
    {
        int idx = buffer_meta_pos[i];
        if (idx != results_left - 1)
        {
            SearchResultMeta tmp = results_meta[idx];
            results_meta[idx] = results_meta[results_left - 1];
            results_meta[results_left - 1] = tmp;
        }
        --results_left;
    }

    ctx->results_left = results_left;
    ctx->results_processed += buf_size;
    ctx->last_page += 1;
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
        uint8_t *ids2;
        uint8_t *ids3;
        size_t n;

        if (read_tsv_pairs(tsv, &texts, &ids, &ids2, &ids3, &n, read_fourth_col) != 0)
            return 1;
        int rc;
        if (read_fourth_col)
            rc = index_build_from_pairs(out, texts, ids, ids2, ids3, n);
        else
            rc = index_build_from_pairs(out, texts, ids, ids2, NULL, n);
        free_pairs(texts, ids, ids2, ids3, n);
        if (read_fourth_col)
            return rc == 0 ? 0 : 1;
    }

    /* ---------- search ---------- */
    else if (strcmp(argv[1], "search") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "search <query> \n");
            return 1;
        }

        const char *query_file = argv[2];

        char *query = read_utf8_file(query_file);
        if (!query)
        {
            fprintf(stderr, "failed read query file\n");
            return 1;
        }

        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);

        bool languages[KOTOBA_LANG_COUNT] = {0};
        languages[KOTOBA_LANG_EN] = true;
        languages[KOTOBA_LANG_ES] = true;

        int page_size = 10;
        struct SearchContext ctx;
        init_search_context(&ctx, languages, &d, page_size);

        query_results(&ctx, query);
        
        query_next_page(&ctx);
        query_next_page(&ctx);
        printf("Results:\n");
        for (uint32_t i = 0; i < ctx.results_processed; ++i)
        {
            print_entry(&d, ctx.results_doc_ids[i]);
        }
        

        return 0;
    }
    else if (strcmp(argv[1], "test") == 0)
    {
        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);

            // Test reading inverted index for "taberu"
            InvertedIndex reading_idx;
            if (index_load("readings.idx", &reading_idx) != 0) {
                fprintf(stderr, "Failed to load readings.idx\n");
                return 1;
            }

            if (argc != 3)
            {
                fprintf(stderr, "test <query_file>\n");
                return 1;
            }
            const char *query_file = argv[2];

            char *query = read_utf8_file(query_file);
            if (!query)
            {
                fprintf(stderr, "failed read query file\n");
                return 1;
            }            

            char mixed[256];
            mixed_to_hiragana(query, mixed, sizeof(mixed));
            printf("Mixed hiragana: %s\n", mixed);


            uint32_t hashes[SEARCH_MAX_QUERY_HASHES];
            int hcount = query_gram_hashes(mixed, hashes, SEARCH_MAX_QUERY_HASHES);

            PostingRef results_buffer[SEARCH_MAX_RESULTS];
            int rcount = index_intersect_postings(
                &reading_idx,
                hashes,
                hcount,
                results_buffer,
                SEARCH_MAX_RESULTS);

            printf("Results for %s in readings.idx: %d\n", query, rcount);
            for (int i = 0; i < rcount; ++i) {
                const entry_bin *e = kotoba_entry(&d, results_buffer[i].p->doc_id);
                int r_elem_id = results_buffer[i].p->meta1;
                const r_ele_bin *r_ele = kotoba_r_ele(&d, e, r_elem_id);
                kotoba_str reb = kotoba_reb(&d, r_ele);
                printf("Entry %u, reading[%d]: %.*s\n", results_buffer[i].p->doc_id, r_elem_id, reb.len, reb.ptr);
            }

            index_unload(&reading_idx);

        return 0;
    }

    /* ---------- unknown ---------- */
    else
    {
        fprintf(stderr, "unknown command\n");
        return 1;
    }
}
