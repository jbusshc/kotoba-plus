#include "../include/index_search.h"

#define SEARCH_MAX_RESULTS 20000
#define SEARCH_MAX_QUERY_HASHES 128
#define DEFAULT_PAGE_SIZE 16

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

int word_contains_q(const char *word, int wlen,
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

void sort_scores(SearchResultMeta *a, int n)
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

    ctx->jp_invx = malloc(sizeof(InvertedIndex));
    ctx->gloss_invxs = calloc(KOTOBA_LANG_COUNT, sizeof(InvertedIndex *));
    if (!ctx->jp_invx || !ctx->gloss_invxs)
    {
        fprintf(stderr, "Memory allocation failed in init_search_context\n");
        exit(1);
    }

    if (index_load("jp.invx", ctx->jp_invx) != 0)
    {
        fprintf(stderr, "Failed to load jp.invx\n");
        exit(1);
    }

    // Only allocate and load gloss indexes for active languages
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        if (ctx->is_gloss_active[i])
        {
            ctx->gloss_invxs[i] = malloc(sizeof(InvertedIndex));
            if (!ctx->gloss_invxs[i])
            {
                fprintf(stderr, "Memory allocation failed for gloss_invxs[%d]\n", i);
                exit(1);
            }
            char fname[64];
            // You may want to use a mapping for filenames instead of this switch
            switch (i)
            {
            case KOTOBA_LANG_EN:
                strcpy(fname, "gloss_en.invx");
                break;
            case KOTOBA_LANG_ES:
                strcpy(fname, "gloss_es.invx");
                break;
            // Add more cases as needed for other languages
            default:
                snprintf(fname, sizeof(fname), "gloss_%d.invx", i);
                break;
            }
            if (index_load(fname, ctx->gloss_invxs[i]) != 0)
            {
                fprintf(stderr, "Failed to load %s\n", fname);
                free(ctx->gloss_invxs[i]);
                ctx->gloss_invxs[i] = NULL;
            }
        }
        else
        {
            ctx->gloss_invxs[i] = NULL;
        }
    }

    ctx->results_doc_ids = malloc(sizeof(uint32_t) * SEARCH_MAX_RESULTS);
    ctx->results = malloc(sizeof(SearchResultMeta) * SEARCH_MAX_RESULTS);
    ctx->results_buffer = malloc(sizeof(PostingRef) * SEARCH_MAX_RESULTS);

    if (!ctx->results_doc_ids || !ctx->results || !ctx->results_buffer)
    {
        fprintf(stderr, "Memory allocation failed for results buffers\n");
        exit(1);
    }
}

void free_search_context(struct SearchContext *ctx)
{
    if (ctx->jp_invx)
    {
        index_unload(ctx->jp_invx);
    }

    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        if (ctx->gloss_invxs[i])
        {
            index_unload(ctx->gloss_invxs[i]);
            ctx->gloss_invxs[i] = NULL;
        }
    }
    free(ctx->results_doc_ids);
    free(ctx->results);
    free(ctx->results_buffer);

    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
    {
        if (ctx->gloss_invxs[i])
        {
            free(ctx->gloss_invxs[i]);
        }
    }
    free(ctx->gloss_invxs);
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
    int hcount = query_gram_hashes_mode(query, GRAM_GLOSS_AUTO, hashes, SEARCH_MAX_QUERY_HASHES);
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
    int mixed_hcount = query_gram_hashes_mode(mixed, GRAM_JP, mixed_hashes, SEARCH_MAX_QUERY_HASHES);
    InvertedIndex *jp_invx = ctx->jp_invx;
    InvertedIndex **gloss_invxs = ctx->gloss_invxs;
    kotoba_dict *dict = ctx->dict;
    PostingRef *results_buffer = ctx->results_buffer;
    SearchResultMeta *results_meta = ctx->results;
    int total_results = 0;
    // Search Kanji index if query contains Kanji

    if (jp_invx)
    {
        int base = total_results;
        int out_cap = SEARCH_MAX_RESULTS - base;
        if (out_cap > 0)
        {
            int rcount = index_intersect_postings(
                jp_invx,
                mixed_hashes,
                mixed_hcount,
                results_buffer + base,
                out_cap);

            int write = base;
            for (int i = 0; i < rcount; ++i)
            {
                PostingRef *pr = &results_buffer[base + i];

                const entry_bin *e = kotoba_entry(dict, pr->p->doc_id);
                kotoba_str str;
                int elem_id = pr->p->meta1;
                int type = pr->p->meta2;
                if (type == TYPE_KANJI)
                {
                    const k_ele_bin *k_ele = kotoba_k_ele(dict, e, elem_id);
                    str = kotoba_keb(dict, k_ele);
                }
                else
                {
                    const r_ele_bin *r_ele = kotoba_r_ele(dict, e, elem_id);
                    str = kotoba_reb(dict, r_ele);
                }

                if (mixed_q.len > str.len)
                    continue;

                if (write != base + i)
                    results_buffer[write] = *pr;

                results_meta[write].results_idx = write;
                results_meta[write].score = (uint16_t)str.len;
                if (type == TYPE_KANJI)
                    results_meta[write].type = 0; // kanji
                else
                    results_meta[write].type = 1; // reading

                write++;
            }
            total_results = write;
        }
    }

    if (total_results < SEARCH_MAX_RESULTS)
    {
        // Search gloss indexes
        for (int lang = 0; lang < KOTOBA_LANG_COUNT; ++lang)
        {
            if (ctx->is_gloss_active[lang])
            {
                InvertedIndex *gloss_idx = ctx->gloss_invxs[lang];
                if (gloss_idx)
                {

                    int base = total_results;
                    int out_cap = SEARCH_MAX_RESULTS - total_results;

                    if (out_cap <= 0)
                        break;

                    int rcount = index_intersect_postings(
                        gloss_idx,
                        hashes,
                        hcount,
                        results_buffer + base,
                        out_cap);

                    int write = base;

                    for (int i = 0; i < rcount; ++i)
                    {
                        PostingRef *pr = &results_buffer[base + i];
                        const entry_bin *e = kotoba_entry(dict, pr->p->doc_id);

                        int sense_id = pr->p->meta1;
                        int gloss_id = pr->p->meta2;

                        const sense_bin *s = kotoba_sense(dict, e, sense_id);

                        kotoba_str gloss = kotoba_gloss(dict, s, gloss_id);

                        if (q.len > gloss.len)
                        {
                            continue;
                        }

                        if (write != base + i)
                            results_buffer[write] = *pr;

                        results_meta[write].results_idx = write;
                        results_meta[write].score = (uint16_t)gloss.len;
                        results_meta[write].type = 2; // gloss

                        write++;
                    }

                    total_results = write;
                }
            }
        }
    }
    // end
    ctx->query = q;
    ctx->results_left = total_results;
    ctx->last_page = 0;
    ctx->results_processed = 0;
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
    int buffer_meta_pos[page_size]; // 🔥 POSICIÓN REAL EN results_meta
    int buf_size = 0;
    int worst_idx = 0;

    query_t q = ctx->query;
    char mixed[256];
    mixed_to_hiragana(q.s, mixed, sizeof(mixed));
    query_t mixed_q = {.s = mixed, .len = (uint8_t)strlen(mixed), .first = mixed[0]};

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
            results_buffer[results_meta[i].results_idx].p->doc_id);

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
                continue; // ✅ FIX
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

void warm_up(struct SearchContext *ctx)
{

    size_t pages = 4; // 16 KB
    for (size_t off = 0; off < pages * 4096; off += 4096)
    {
        if (off < ctx->jp_invx->mapped_size)
        {
            volatile char x = ((const char *)ctx->jp_invx->base)[off];
        }
        for (int lang = 0; lang < KOTOBA_LANG_COUNT; ++lang)
        {
            if (ctx->gloss_invxs[lang] && off < ctx->gloss_invxs[lang]->mapped_size)
            {
                volatile char y = ((const char *)ctx->gloss_invxs[lang]->base)[off];
            }
        }
    }

    /* Use ctx buffers for warm-up queries */
    static const char *warm_queries_jp[] = {
        // JP (kanji / kana)
        "の",
        "に",
        "を",
        "は",
        "が",
        "する",
        "ある",
        "う",
        "る",
        "く",
        "す",
        "つ",
        "ぬ",
        "む",
        "ぶ",
        "ぷ",
        "ぐ", // all u terminations
        "人",
    };
    static const char *warm_queries_en[] = {
        // Gloss (EN)
        "the", "of ", "and", "to ", "in "};

    const size_t jp_nq = sizeof(warm_queries_jp) / sizeof(warm_queries_jp[0]);
    const size_t en_nq = sizeof(warm_queries_en) / sizeof(warm_queries_en[0]);

    uint32_t *hashes = ctx->results_doc_ids;          // Reuse ctx->results_doc_ids as a buffer
    PostingRef *results_buffer = ctx->results_buffer; // Use ctx->results_buffer
    int test_size = 32;

    for (size_t i = 0; i < jp_nq; ++i)
    {
        const char *q = warm_queries_jp[i];
        int hcount = query_gram_hashes_mode(q, GRAM_JP, hashes, SEARCH_MAX_QUERY_HASHES);
        index_intersect_postings(
            ctx->jp_invx,
            hashes,
            hcount,
            results_buffer,
            test_size);
    }

    for (size_t i = 0; i < en_nq; ++i)
    {
        const char *q = warm_queries_en[i];
        int hcount = query_gram_hashes_mode(q, GRAM_GLOSS_AUTO, hashes, SEARCH_MAX_QUERY_HASHES);
        for (int lang = 0; lang < KOTOBA_LANG_COUNT; ++lang)
        {
            InvertedIndex *g = ctx->gloss_invxs[lang];
            if (g)
            {
                index_intersect_postings(
                    g,
                    hashes,
                    hcount,
                    results_buffer,
                    test_size);
            }
        }
    }
}