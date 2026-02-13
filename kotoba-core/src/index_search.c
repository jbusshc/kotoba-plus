#include "../include/index_search.h"

#define SEARCH_MAX_RESULTS 100
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

    ctx->trie_ctx = malloc(sizeof(TrieContext));
    if (!ctx->trie_ctx)
    {
        fprintf(stderr, "Memory allocation failed for TrieContext\n");
        exit(1);
    }
    ctx->queries_buffer = malloc(QUERY_BUFFER_SIZE);
    if (!ctx->queries_buffer)
    {
        fprintf(stderr, "Memory allocation failed for queries_buffer\n");
        exit(1);
    }
    ctx->mixed_query = ctx->queries_buffer;                   // reuse buffer
    ctx->variant_query = ctx->queries_buffer + MAX_QUERY_LEN; // reuse buffer
    build_trie(ctx->trie_ctx);

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

static inline uint32_t vowel_to_hiragana(int vowel)
{
    switch (vowel)
    {
    case 'a':
        return 0x3042; // あ
    case 'i':
        return 0x3044; // い
    case 'u':
        return 0x3046; // う
    case 'e':
        return 0x3048; // え
    case 'o':
        return 0x304A; // お
    default:
        return 0;
    }
}

static int search_jp_query(
    struct SearchContext *ctx,
    const char *query_str,
    PostingRef *results_buffer,
    SearchResultMeta *results_meta,
    int base,
    int max_results)
{
    if (!ctx->jp_invx)
        return base;

    query_t q;
    q.s = query_str;
    q.len = (uint8_t)strlen(query_str);
    q.first = query_str[0];

    uint32_t hashes[SEARCH_MAX_QUERY_HASHES];
    int hcount = query_gram_hashes_mode(query_str, GRAM_JP, hashes, SEARCH_MAX_QUERY_HASHES);
    if (hcount == 0)
        return base;

    int out_cap = max_results - base;
    if (out_cap <= 0)
        return base;

    int rcount = index_intersect_postings(
        ctx->jp_invx,
        hashes,
        hcount,
        results_buffer + base,
        out_cap);

    kotoba_dict *dict = ctx->dict;

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

        // ✅ CORRECTO: usamos la longitud de ESTA query
        if (q.len > str.len)
        {
            continue;
        }

        if (write != base + i)
            results_buffer[write] = *pr;

        results_meta[write].results_idx = write;
        results_meta[write].score = (uint16_t)str.len;
        results_meta[write].type = (type == TYPE_KANJI) ? 0 : 1;

        write++;
    }
    return write;
}

void query_results(struct SearchContext *ctx, const char *query)
{
    reset_search_context(ctx);

    query_t q;
    q.s = query;
    q.len = (uint8_t)strlen(query);
    q.first = query[0];

    uint32_t gloss_hashes[SEARCH_MAX_QUERY_HASHES];
    int gloss_hcount = query_gram_hashes_mode(
        query,
        GRAM_GLOSS_AUTO,
        gloss_hashes,
        SEARCH_MAX_QUERY_HASHES);

    PostingRef *results_buffer = ctx->results_buffer;
    SearchResultMeta *results_meta = ctx->results;
    kotoba_dict *dict = ctx->dict;

    int total_results = 0;

    int input_type = get_input_type(query);

    // =========================================================
    // GLOSS SEARCH 
    // =========================================================

    if (gloss_hcount > 0 && (input_type & INPUT_TYPE_ROMAJI))
    {
        for (int lang = 0; lang < KOTOBA_LANG_COUNT; ++lang)
        {
            if (!ctx->is_gloss_active[lang])
                continue;

            InvertedIndex *gloss_idx = ctx->gloss_invxs[lang];
            if (!gloss_idx)
                continue;

            int base = total_results;
            int out_cap = SEARCH_MAX_RESULTS - base;

            if (out_cap <= 0)
                break;

            int rcount = index_intersect_postings(
                gloss_idx,
                gloss_hashes,
                gloss_hcount,
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
                    continue;

                if (write != base + i)
                    results_buffer[write] = *pr;

                results_meta[write].results_idx = write;
                results_meta[write].score = (uint16_t)gloss.len;
                results_meta[write].type = 2; // gloss

                write++;
            }

            total_results = write;

            if (total_results >= SEARCH_MAX_RESULTS)
                break;
        }
    }

    // =========================================================
    // JP SEARCH (SIEMPRE INTENTAR, independiente de gloss)
    // =========================================================

    if (ctx->jp_invx && total_results < SEARCH_MAX_RESULTS)
    {
        mixed_to_hiragana(ctx->trie_ctx, query, ctx->mixed_query, MAX_QUERY_LEN);

        // 1️⃣ Búsqueda base normalizada
        total_results = search_jp_query(
            ctx,
            ctx->mixed_query,
            results_buffer,
            results_meta,
            total_results,
            SEARCH_MAX_RESULTS);

        // 2️⃣ Variante con vowel prolongation mark (ー)
        if (total_results < SEARCH_MAX_RESULTS)
        {
            vowel_prolongation_mark(ctx->mixed_query, ctx->variant_query, MAX_QUERY_LEN);
            if (strcmp(ctx->variant_query, ctx->mixed_query) != 0)
            {
                total_results = search_jp_query(
                    ctx,
                    ctx->variant_query,
                    results_buffer,
                    results_meta,
                    total_results,
                    SEARCH_MAX_RESULTS);
            }
        }
    }

    // =========================================================
    // FINALIZACIÓN
    // =========================================================

    ctx->query = q;
    ctx->results_left = total_results;
    ctx->last_page = 0;
    ctx->results_processed = 0;
}

void query_next_page(struct SearchContext *ctx)
{
    if (ctx->results_left == 0)
    {
        return;
    }

    kotoba_dict *dict = ctx->dict;
    PostingRef *results_buffer = ctx->results_buffer;
    SearchResultMeta *results_meta = ctx->results;

    uint32_t original_results_left = ctx->results_left;
    uint32_t page_size = ctx->page_size;
    uint32_t last_page = ctx->last_page;

    if (page_size > original_results_left)
        page_size = original_results_left;

    // ---------- queries ----------
    query_t q = ctx->query;

    query_t mixed_q = {
        .s = ctx->mixed_query,
        .len = (uint8_t)strlen(ctx->mixed_query),
        .first = ctx->mixed_query[0]};

    query_t variant_q = {
        .s = ctx->variant_query,
        .len = (uint8_t)strlen(ctx->variant_query),
        .first = ctx->variant_query[0]};

    int using_variant = strcmp(ctx->variant_query, ctx->mixed_query) != 0;

    // ---------- top-K buffer (ordenado ascendente por score) ----------
    SearchResultMeta buffer[page_size];
    int buffer_meta_pos[page_size];
    int buf_size = 0;

    // =========================================================
    // 1️⃣ FILTRAR + TOP-K (ordenado)
    // =========================================================

    for (uint32_t i = 0; i < original_results_left; ++i)
    {
        SearchResultMeta *meta = &results_meta[i];
        int score = meta->score;

        const entry_bin *e =
            kotoba_entry(dict,
                         results_buffer[meta->results_idx].p->doc_id);

        int type = meta->type;
        int meta1 = results_buffer[meta->results_idx].p->meta1;
        int meta2 = results_buffer[meta->results_idx].p->meta2;

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

        /*
        int contains = word_contains_q(str.ptr, str.len, query);

        if (!contains && using_variant && (type == 0 || type == 1))
            contains = word_contains_q(str.ptr, str.len, &variant_q);

        if (!contains)
            continue;
        */

        // ---------- top-K ordenado ----------
        if (buf_size == 0)
        {
            buffer[0] = *meta;
            buffer_meta_pos[0] = i;
            buf_size = 1;
        }
        else if (buf_size < page_size)
        {
            int pos = buf_size;

            while (pos > 0 && buffer[pos - 1].score > score)
            {
                buffer[pos] = buffer[pos - 1];
                buffer_meta_pos[pos] = buffer_meta_pos[pos - 1];
                pos--;
            }

            buffer[pos] = *meta;
            buffer_meta_pos[pos] = i;
            buf_size++;
        }
        else if (score < buffer[buf_size - 1].score)
        {
            int pos = buf_size - 1;

            while (pos > 0 && buffer[pos - 1].score > score)
            {
                buffer[pos] = buffer[pos - 1];
                buffer_meta_pos[pos] = buffer_meta_pos[pos - 1];
                pos--;
            }

            buffer[pos] = *meta;
            buffer_meta_pos[pos] = i;
        }
    }

    if (buf_size == 0)
    {
        ctx->results_left = 0;
        return;
    }

    // =========================================================
    // 2️⃣ YA ESTÁ ORDENADO → escribir página
    // =========================================================

    int offset = last_page * ctx->page_size;

    for (int i = 0; i < buf_size; ++i)
    {
        ctx->results_doc_ids[offset + i] =
            results_buffer[buffer[i].results_idx].p->doc_id;
    }

    // =========================================================
    // 3️⃣ ELIMINAR SOLO LOS SELECCIONADOS
    // =========================================================

    for (int i = buf_size - 1; i >= 0; --i)
    {
        int idx = buffer_meta_pos[i];

        if (idx != ctx->results_left - 1)
        {
            SearchResultMeta tmp = results_meta[idx];
            results_meta[idx] = results_meta[ctx->results_left - 1];
            results_meta[ctx->results_left - 1] = tmp;
        }

        ctx->results_left--;
    }

    ctx->results_processed += buf_size;
    ctx->last_page++;
}

void warm_up(struct SearchContext *ctx)
{

    size_t pages = 128; // 16 KB
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

}