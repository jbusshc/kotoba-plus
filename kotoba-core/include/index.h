#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IDX_MAGIC   0x494E5658u  // "INVX"
#define IDX_VERSION 3u          // bumped: gram mode supported at build/query

#include "kotoba.h"

/* =========================
   ON-DISK STRUCTURES
   ========================= */

/* on-disk header */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t term_count;
    uint32_t posting_count;
} __attribute__((packed)) IndexHeader;

/* a term: hash of n-gram */
typedef struct {
    uint32_t gram_hash;
    uint32_t postings_offset; /* index into postings array */
    uint32_t postings_count;
} __attribute__((packed)) Term;

/* posting:
   - kanji/readings index:
       meta1 = k_ele or r_ele index
       meta2 = 0
   - gloss index:
       meta1 = sense_id
       meta2 = gloss_id
*/
typedef struct {
    uint32_t doc_id;
    uint8_t  meta1;
    uint8_t  meta2;
    uint8_t  reserved[2];
} __attribute__((packed)) Posting;

/* runtime mmap view */
typedef struct {
    const IndexHeader *hdr;
    const Term        *terms;
    const Posting     *postings;
    const void        *base;
    size_t             mapped_size;
} InvertedIndex;

/* =========================
   GRAM MODES
   ========================= */

typedef enum {
    GRAM_UNIGRAM = 1,
    GRAM_BIGRAM  = 2,
    GRAM_TRIGRAM = 3,

    /* Gloss auto mode:
       len == 1 → unigram
       len == 2 → bigram
       len >= 3 → trigram
    */
    GRAM_GLOSS_AUTO = 0,
    GRAM_JP = 4
} GramMode;



/* =========================
   INDEX BUILD
   ========================= */

/*
   Build an inverted index from text/doc_id pairs.

   - texts[]      : array of UTF-8 strings
   - doc_ids[]    : user document ids
   - meta1/meta2  : optional metadata arrays
   - gram_mode    : how grams are generated

   Typical usage:
     - kanji/readings index: GRAM_UNIGRAM | GRAM_BIGRAM (caller decides)
     - gloss index:          GRAM_GLOSS_AUTO
*/
KOTOBA_API int index_build_from_pairs(
    const char   *out_path,
    const char  **texts,
    const uint32_t *doc_ids,
    const uint8_t  *meta1,    /* optional */
    const uint8_t  *meta2,    /* optional */
    size_t          count,
    GramMode        gram_mode
);

/* =========================
   LOAD / UNLOAD
   ========================= */

KOTOBA_API int  index_load(const char *path, InvertedIndex *idx);
KOTOBA_API void index_unload(InvertedIndex *idx);

/* =========================
   LOOKUP
   ========================= */

/* lookup a gram hash -> Term* (binary search) */
KOTOBA_API const Term *index_find_term(
    const InvertedIndex *idx,
    uint32_t hash
);

/* copy postings of a term */
KOTOBA_API size_t index_term_postings(
    const InvertedIndex *idx,
    const Term *t,
    Posting *out_postings,
    size_t out_cap
);

/* intersect hashes -> doc_ids */
KOTOBA_API size_t index_intersect_hashes(
    const InvertedIndex *idx,
    const uint32_t *hashes,
    size_t hcount,
    uint32_t *out,
    size_t out_cap
);

/* posting reference */
typedef struct {
    const Posting *p;   /* pointer directly into index */
} PostingRef;

/* intersect hashes -> postings */
KOTOBA_API size_t index_intersect_postings(
    const InvertedIndex *idx,
    const uint32_t *hashes,
    size_t hcount,
    PostingRef *out,
    size_t out_cap
);

/* =========================
   QUERY GRAM GENERATION
   ========================= */

/*
   Generate gram hashes for a query string.

   gram_mode:
     - GRAM_UNIGRAM / BIGRAM / TRIGRAM : force mode
     - GRAM_GLOSS_AUTO:
         len 1 → uni
         len 2 → bi
         len ≥3 → tri
     - GRAM_JP 
        1 char -> uni
        ≥2 chars -> bi
*/
KOTOBA_API size_t query_gram_hashes_mode(
    const char *q,
    GramMode mode,
    uint32_t *out_hashes,
    size_t max
);

/* =========================
   INTERNAL (shared helpers)
   ========================= */

/* callback type */
typedef void (*gram_cb)(const uint8_t *p, size_t len, void *ud);

/*
   Generate grams of a specific size (1,2,3).
   UTF-8 aware, sliding window.
*/


/* capture callback ud */
struct QGramUD {
    uint32_t *out;
    size_t    max;
    size_t    count;
};


/* =========================
   API
   ========================= */

/* utils */
KOTOBA_API uint32_t fnv1a(const void *data, size_t len);
KOTOBA_API size_t   utf8_char_len(uint8_t c);
KOTOBA_API size_t   utf8_strlen(const char *s);
KOTOBA_API void utf8_ngrams_cb(const char *s, int n, gram_cb cb, void *ud);