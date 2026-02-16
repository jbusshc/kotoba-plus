#include "../include/index_search.h"

#define SEARCH_MAX_RESULTS 1600
#define SEARCH_MAX_QUERY_HASHES 128
#define DEFAULT_PAGE_SIZE 16
#define PAGE_SIZE_MAX 128

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

static inline int word_contains_q(const char *word, int wlen,
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

void init_search_context(struct SearchContext *ctx,
                         bool *glosses_active,
                         kotoba_dict *dict,
                         uint32_t page_size)
{
    memset(ctx, 0, sizeof(struct SearchContext));
    ctx->dict = dict;
    ctx->page_size = page_size > 0 && page_size <= PAGE_SIZE_MAX ? page_size : DEFAULT_PAGE_SIZE;
    ctx->results_left = 0;
    ctx->page_result_count = 0;
    ctx->prolongation_mark_flag = 0;

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

    ctx->results_doc_ids = malloc(sizeof(uint32_t) * PAGE_SIZE_MAX);
    ctx->results_buffer = malloc(sizeof(PostingRef) * SEARCH_MAX_RESULTS);

    if (!ctx->results_doc_ids || !ctx->results_buffer)
    {
        fprintf(stderr, "Memory allocation failed for results buffers\n");
        exit(1);
    }
    ctx->results.results_idx = malloc(sizeof(uint32_t) * SEARCH_MAX_RESULTS);
    ctx->results.score = malloc(sizeof(uint8_t) * SEARCH_MAX_RESULTS);
    ctx->results.type = malloc(sizeof(uint8_t) * SEARCH_MAX_RESULTS);
    if (!ctx->results.results_idx || !ctx->results.score || !ctx->results.type)
    {
        fprintf(stderr, "Memory allocation failed for SearchResultMeta arrays\n");
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
    free(ctx->results.results_idx);
    free(ctx->results.score);
    free(ctx->results.type);
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
    ctx->page_result_count = 0;
    ctx->prolongation_mark_flag = 0;
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

static inline void scoring(query_t *query, kotoba_str *str, uint8_t *out_score, int type)
{
    uint8_t exact = (query->len == str->len && memcmp(query->s, str->ptr, query->len) == 0);
    uint8_t is_jp = (type == TYPE_KANJI || type == TYPE_READING) ? 1 : 0;
    uint8_t common = 0; // TODO: implement common word detection

    uint8_t diff = (str->len > query->len)
                       ? (str->len - query->len)
                       : (query->len - str->len);

    if (diff > 31)
        diff = 31;

    uint8_t proximity = 31 - diff;

    *out_score = (exact << 7) | (is_jp << 6) | (common << 5) | proximity;
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
    int hcount = query_gram_hashes_mode(query_str, GRAM_JP_ALL, hashes, SEARCH_MAX_QUERY_HASHES);
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

        ctx->results.results_idx[write] = write;
        ctx->results.type[write] = (type == TYPE_KANJI) ? TYPE_KANJI : TYPE_READING;
        scoring(&q, &str, &ctx->results.score[write], type);

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
    kotoba_dict *dict = ctx->dict;

    int total_results = 0;

    int input_type = get_input_type(query);

    // SEARCH ORDER DEPENDS ON QUERY LENGTH
    if (q.len >= 3)
    {
        // JP SEARCH FIRST
        if (ctx->jp_invx)
        {
            mixed_to_hiragana(ctx->trie_ctx, query, ctx->mixed_query, MAX_QUERY_LEN);

            total_results = search_jp_query(
                ctx,
                ctx->mixed_query,
                results_buffer,
                &ctx->results,
                total_results,
                SEARCH_MAX_RESULTS);

            if (total_results < SEARCH_MAX_RESULTS)
            {
                uint8_t prolongation_mark_flag = 0;
                vowel_prolongation_mark(ctx->mixed_query, ctx->variant_query, MAX_QUERY_LEN, &prolongation_mark_flag);
                if (prolongation_mark_flag)
                {
                    ctx->prolongation_mark_flag = 1;
                    total_results = search_jp_query(
                        ctx,
                        ctx->variant_query,
                        results_buffer,
                        &ctx->results,
                        total_results,
                        SEARCH_MAX_RESULTS);
                }
            }
        }

        // GLOSS SEARCH SECOND
        if (gloss_hcount > 0 && (input_type & INPUT_TYPE_ROMAJI) && total_results < SEARCH_MAX_RESULTS)
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

                    ctx->results.results_idx[write] = write;
                    ctx->results.type[write] = TYPE_GLOSS;
                    scoring(&q, &gloss, &ctx->results.score[write], TYPE_GLOSS);

                    write++;
                }

                total_results = write;

                if (total_results >= SEARCH_MAX_RESULTS)
                    break;
            }
        }
    }
    else
    {
        // GLOSS SEARCH FIRST
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

                    ctx->results.results_idx[write] = write;
                    ctx->results.type[write] = TYPE_GLOSS;
                    scoring(&q, &gloss, &ctx->results.score[write], TYPE_GLOSS);

                    write++;
                }

                total_results = write;

                if (total_results >= SEARCH_MAX_RESULTS)
                    break;
            }
        }

        // JP SEARCH SECOND
        if (ctx->jp_invx && total_results < SEARCH_MAX_RESULTS)
        {
            mixed_to_hiragana(ctx->trie_ctx, query, ctx->mixed_query, MAX_QUERY_LEN);

            total_results = search_jp_query(
                ctx,
                ctx->mixed_query,
                results_buffer,
                &ctx->results,
                total_results,
                SEARCH_MAX_RESULTS);

            if (total_results < SEARCH_MAX_RESULTS)
            {
                uint8_t prolongation_mark_flag = 0;
                vowel_prolongation_mark(ctx->mixed_query, ctx->variant_query, MAX_QUERY_LEN, &prolongation_mark_flag);
                if (prolongation_mark_flag)
                {
                    ctx->prolongation_mark_flag = 1;
                    total_results = search_jp_query(
                        ctx,
                        ctx->variant_query,
                        results_buffer,
                        &ctx->results,
                        total_results,
                        SEARCH_MAX_RESULTS);
                }
            }
        }
    }

    // =========================================================
    // FINALIZACIÓN
    // =========================================================

    ctx->query = q;
    ctx->results_left = total_results;
}

static inline void delete_result(uint32_t idx, uint32_t *results_left, SearchResultMeta *results_meta)
{
    if (idx >= *results_left)
    {
        return;
    }

    // Simple deletion by swapping with the last element
    uint32_t last_idx = *results_left - 1;
    if (idx != last_idx)
    {
        results_meta->results_idx[idx] = results_meta->results_idx[last_idx];
        results_meta->score[idx] = results_meta->score[last_idx];
        results_meta->type[idx] = results_meta->type[last_idx];
    }
    (*results_left)--;
}

static inline int hard_filter(const kotoba_dict *dict, TrieContext *trie_ctx, PostingRef *pr, query_t *normal_q, query_t *mixed_q, query_t *variant_q, int type, uint8_t using_variant)
{
    static char normalized_str[256];
    const entry_bin *e = kotoba_entry(dict, pr->p->doc_id);
    kotoba_str str;
    query_t *q = mixed_q;

    if (type == TYPE_KANJI)
    {
        const k_ele_bin *k_ele = kotoba_k_ele(dict, e, pr->p->meta1);
        str = kotoba_keb(dict, k_ele);
    }
    else if (type == TYPE_READING)
    {
        const r_ele_bin *r_ele = kotoba_r_ele(dict, e, pr->p->meta1);
        str = kotoba_reb(dict, r_ele);
    }
    else
    {
        const sense_bin *s = kotoba_sense(dict, e, pr->p->meta1);
        str = kotoba_gloss(dict, s, pr->p->meta2);
        q = normal_q; // for glosses we want to match the original query, not the mixed one
    }

    if (word_contains_q(str.ptr, str.len, q))
        return 1;

    if (using_variant && word_contains_q(str.ptr, str.len, variant_q))
        return 1;

    return 0;
}

// new version with partial sorting at top k, no hard filter, and scoring based on new score uint_8 binary instead of length (score layout defined in index_search.h)
void query_next_page(struct SearchContext *ctx)
{
    if (ctx->results_left == 0 || ctx->page_size == 0)
        return;

    uint32_t k = ctx->page_size;
    if (k > ctx->results_left)
        k = ctx->results_left;

    if (k == 0)
        return;

    PostingRef *results_buffer = ctx->results_buffer;
    SearchResultMeta *meta = &ctx->results;

    uint8_t *score = meta->score;
    uint8_t *type = meta->type;
    uint32_t *results_idx = meta->results_idx;

    // ---------- QUERIES ----------
    query_t q = ctx->query;

    query_t mixed_q = {
        .s = ctx->mixed_query,
        .len = (uint8_t)strlen(ctx->mixed_query),
        .first = ctx->mixed_query[0]};

    query_t variant_q = {
        .s = ctx->variant_query,
        .len = (uint8_t)strlen(ctx->variant_query),
        .first = ctx->variant_query[0]};

    uint8_t using_variant = ctx->prolongation_mark_flag;

    // ---------- TOP-K BUFFERS ----------
    uint8_t topk_scores[k];
    uint32_t topk_meta_index[k]; // ← índice dentro de meta arrays

    uint32_t topk_size = 0;
    uint32_t min_pos = 0;
    uint8_t min_score = 0;

    // ---------- SINGLE PASS ----------
    uint32_t i = 0;
    while (i < ctx->results_left)
    {
        uint8_t current_score = score[i];

        if (topk_size == k && current_score <= min_score)
        {
            i++;
            continue;
        }

        // 🔥 CORRECT ACCESS
        PostingRef *posting =
            &results_buffer[results_idx[i]];

        if (!hard_filter(ctx->dict,
                         ctx->trie_ctx,
                         posting,
                         &q,
                         &mixed_q,
                         &variant_q,
                         type[i],
                         using_variant))
        {
            delete_result(i, &ctx->results_left, meta);
            continue; // DO NOT i++
        }

        // ---------- INSERT ----------
        if (topk_size < k)
        {
            topk_scores[topk_size] = current_score;
            topk_meta_index[topk_size] = i;

            if (topk_size == 0 || current_score < min_score)
            {
                min_score = current_score;
                min_pos = topk_size;
            }

            topk_size++;
        }
        else
        {
            topk_scores[min_pos] = current_score;
            topk_meta_index[min_pos] = i;

            // recompute min
            min_score = topk_scores[0];
            min_pos = 0;

            for (uint32_t j = 1; j < k; j++)
            {
                if (topk_scores[j] < min_score)
                {
                    min_score = topk_scores[j];
                    min_pos = j;
                }
            }
        }

        i++;
    }

    if (topk_size == 0)
        return;

    // ---------- SORT DESC ----------
    for (uint32_t a = 0; a < topk_size - 1; a++)
    {
        for (uint32_t b = 0; b < topk_size - a - 1; b++)
        {
            if (topk_scores[b] < topk_scores[b + 1])
            {
                uint8_t ts = topk_scores[b];
                uint32_t ti = topk_meta_index[b];

                topk_scores[b] = topk_scores[b + 1];
                topk_meta_index[b] = topk_meta_index[b + 1];

                topk_scores[b + 1] = ts;
                topk_meta_index[b + 1] = ti;
            }
        }
    }

    // ---------- WRITE OUTPUT ----------
    for (uint32_t j = 0; j < topk_size; j++)
    {
        uint32_t meta_i = topk_meta_index[j];
        uint32_t posting_i = results_idx[meta_i];

        ctx->results_doc_ids[j] =
            results_buffer[posting_i].p->doc_id;
    }

    // ---------- SAFE DELETE ----------
    // ordenar topk_meta_index DESC
    for (uint32_t a = 0; a < topk_size - 1; a++)
    {
        for (uint32_t b = 0; b < topk_size - a - 1; b++)
        {
            if (topk_meta_index[b] < topk_meta_index[b + 1])
            {
                uint32_t tmp = topk_meta_index[b];
                topk_meta_index[b] = topk_meta_index[b + 1];
                topk_meta_index[b + 1] = tmp;
            }
        }
    }

    for (uint32_t j = 0; j < topk_size; j++)
    {
        delete_result(topk_meta_index[j],
                      &ctx->results_left,
                      meta);
    }

    ctx->page_result_count = topk_size;
}

/*
OLD
void query_next_page(struct SearchContext *ctx)
{
    if (ctx->results_left == 0)
        return;
    kotoba_dict *dict = ctx->dict;
    PostingRef *results_buffer = ctx->results_buffer;

    uint32_t page_size = ctx->page_size;
    uint32_t last_page = ctx->last_page;

    if (page_size > ctx->results_left)
        page_size = ctx->results_left;

    // ===============================
    // Build queries
    // ===============================

    query_t q = ctx->query;

    query_t mixed_q = {
        .s = ctx->mixed_query,
        .len = (uint8_t)strlen(ctx->mixed_query),
        .first = ctx->mixed_query[0]};

    query_t variant_q = {
        .s = ctx->variant_query,
        .len = (uint8_t)strlen(ctx->variant_query),
        .first = ctx->variant_query[0]};

    uint8_t using_variant = strcmp(ctx->variant_query, ctx->mixed_query) != 0;

    // =========================================================
    // 1️⃣ TOP-K con hard_filter solo si compite
    // =========================================================

    uint8_t topk_scores[page_size];
    uint8_t topk_types[page_size];
    uint32_t topk_indices[page_size];
    uint32_t topk_size = 0;
    uint8_t *len = ctx->results.len;
    uint8_t *type = ctx->results.type;
    uint32_t *results_idx = ctx->results.results_idx;

    uint32_t i = 0;
    while (i < ctx->results_left)
    {
        uint8_t len = ctx->results.len[i];

        // --- 1️⃣ len pruning ---
        if (topk_size == page_size &&
            len >= topk_scores[topk_size - 1])
        {
            i++;
            continue;
        }

        // --- 2️⃣ construir string SOLO si compite ---
        const entry_bin *e =
            kotoba_entry(dict,
                         results_buffer[results_idx[i]].p->doc_id);

        int type = ctx->results.type[i];
        int meta1 = results_buffer[results_idx[i]].p->meta1;
        int meta2 = results_buffer[results_idx[i]].p->meta2;

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

        // --- 3️⃣ hard_filter SOLO si compite ---
        if (!hard_filter(ctx, &str, query, &variant_q, type, using_variant))
        {
            // remover inmediatamente
            uint32_t last = ctx->results_left - 1;

            if (i != last)
            {
                ctx->results.results_idx[i] = ctx->results.results_idx[last];
                ctx->results.len[i] = ctx->results.len[last];
                ctx->results.type[i] = ctx->results.type[last];
            }

            ctx->results_left--;
            continue; // IMPORTANTÍSIMO: no i++
        }

        // --- 4️⃣ insertar ordenado (insertion sort K pequeño) ---
        if (topk_size < page_size)
        {
            int pos = topk_size;

            while (pos > 0 && topk_scores[pos - 1] > len)
            {
                topk_scores[pos] = topk_scores[pos - 1];
                topk_types[pos] = topk_types[pos - 1];
                topk_indices[pos] = topk_indices[pos - 1];
                pos--;
            }

            topk_scores[pos] = len;
            topk_types[pos] = ctx->results.type[i];
            topk_indices[pos] = i;
            topk_size++;
        }
        else
        {
            int pos = topk_size - 1;

            while (pos > 0 && topk_scores[pos - 1] > len)
            {
                topk_scores[pos] = topk_scores[pos - 1];
                topk_types[pos] = topk_types[pos - 1];
                topk_indices[pos] = topk_indices[pos - 1];
                pos--;
            }

            topk_scores[pos] = len;
            topk_types[pos] = ctx->results.type[i];
            topk_indices[pos] = i;
        }
        i++;
    }

    if (topk_size == 0)
        return;

    // =========================================================
    // 2️⃣ ESCRIBIR RESULTADOS
    // =========================================================

    uint32_t offset = last_page * ctx->page_size;

    for (uint32_t i = 0; i < topk_size; i++)
    {
        ctx->results_doc_ids[offset + i] =
            results_buffer[topk_indices[i]].p->doc_id;
    }

    // =========================================================
    // 3️⃣ ELIMINAR SELECCIONADOS (seguro)
    // =========================================================

    for (int k = topk_size - 1; k >= 0; k--)
    {
        uint32_t idx = topk_indices[k];

        if (idx < ctx->results_left)
        {
            if (idx != ctx->results_left - 1)
            {
                ctx->results.results_idx[idx] = ctx->results.results_idx[ctx->results_left - 1];
                ctx->results.len[idx] = ctx->results.len[ctx->results_left - 1];
                ctx->results.type[idx] = ctx->results.type[ctx->results_left - 1];
            }

            ctx->results_left--;
        }
    }

    ctx->results_processed += topk_size;
    ctx->last_page++;
}
*/

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