#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "index.h"
#include "loader.h"
#include "kotoba.h"
#include "kana.h"
#include "viewer.h"

#define TYPE_KANJI 1
#define TYPE_READING 2

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

KOTOBA_API int get_input_type(const char *query);

KOTOBA_API int
word_contains_q(const char *word, int wlen,
                const query_t *q);

typedef struct
{
    uint32_t results_idx; // index in results buffer
    uint16_t score;       // lower is better
    uint16_t type;        // 0=kanji,1=reading,2+=gloss lang index

} SearchResultMeta;

KOTOBA_API void
sort_scores(SearchResultMeta *a, int n);

struct SearchContext
{
    bool is_gloss_active[KOTOBA_LANG_COUNT]; // 28 bytes
    uint8_t _pad0[4];                        // pad to 32 bytes (8-byte aligned)
    query_t query;
    kotoba_dict *dict;
    InvertedIndex *jp_invx;
    InvertedIndex **gloss_invxs;
    PostingRef *results_buffer;
    uint32_t *results_doc_ids;
    SearchResultMeta *results;
    uint32_t results_left;
    uint32_t results_processed;
    uint32_t page_size;
    uint32_t last_page;
};

KOTOBA_API void init_search_context(struct SearchContext *ctx,
                                    bool *glosses_active,
                                    kotoba_dict *dict,
                                    uint32_t page_size);

KOTOBA_API void free_search_context(struct SearchContext *ctx);

KOTOBA_API void reset_search_context(struct SearchContext *ctx);

KOTOBA_API void query_results(struct SearchContext *ctx, const char *query);

KOTOBA_API void query_next_page(struct SearchContext *ctx);

KOTOBA_API
void warm_up(struct SearchContext *ctx);