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
#include "arena.h"

#define TYPE_KANJI 1
#define TYPE_READING 2
#define TYPE_GLOSS 4

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

/* SCORE LAYOUT (uint_8)
    [ E ][ T ][ C ][ PPPPP ]
    E: Exact match (1 bit)
    T: is jp: (1 bit for jp, 0 for gloss (resembles type))
    C: Common word (1 bit)
    PPPPP: length pruning (5 bits, 0-31) (computed as min(31, word_len - query_len))
*/

typedef struct // SoA for better cache locality when sorting
{
    uint32_t *results_idx; // index in results buffer
    uint8_t *score;        // computed score for sorting
    uint8_t *type;         // 0=kanji,1=reading,2+=gloss lang index

} SearchResultMeta;

KOTOBA_API void
sort_scores(SearchResultMeta *a, int n);

#define MAX_QUERY_LEN 256
#define QUERY_BUFFER_SIZE (MAX_QUERY_LEN * 2) // para normalizaciones (hiragana, vowel prolongation mark, etc)
#define SEARCH_MAX_RESULTS 1600
#define SEARCH_MAX_QUERY_HASHES 128
#define DEFAULT_PAGE_SIZE 16
#define PAGE_SIZE_MAX 128

#define ARENA_SIZE ( \
    QUERY_BUFFER_SIZE + \
    (sizeof(uint32_t) * PAGE_SIZE_MAX) + \
    (sizeof(PostingRef) * SEARCH_MAX_RESULTS) + \
    (sizeof(uint32_t) * SEARCH_MAX_RESULTS) + \
    (sizeof(uint8_t) * SEARCH_MAX_RESULTS) + \
    (sizeof(uint8_t) * SEARCH_MAX_RESULTS) + \
    1024 /* margen de seguridad */ \
)

struct SearchContext
{
    bool is_gloss_active[KOTOBA_LANG_COUNT];
    uint8_t _pad0[4];

    query_t query;
    TrieContext *trie_ctx;

    Arena arena;              // 👈 NUEVO (para buffers estáticos)

    char *queries_buffer;
    char *mixed_query;
    char *variant_query;

    kotoba_dict *dict;

    InvertedIndex *jp_invx;
    InvertedIndex **gloss_invxs;

    PostingRef *results_buffer;
    uint32_t *results_doc_ids;

    SearchResultMeta results;

    uint32_t results_left;
    uint32_t page_size;
    uint8_t page_result_count;
    uint8_t prolongation_mark_flag;
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

/*
TO DO:
CHECK duplicates

*/