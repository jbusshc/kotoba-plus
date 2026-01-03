#pragma once
#include <stdint.h>
#include <stddef.h>

#define IDX_MAGIC 0x494E5658u  // "INVX"
#define IDX_VERSION 1u

#include "kotoba.h"

/* on-disk header */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t term_count;
    uint32_t posting_count;
} __attribute__((packed)) IndexHeader;

/* a term: hash of ngram (1-gram or 2-gram) */
typedef struct {
    uint32_t gram_hash;
    uint32_t postings_offset; /* index into postings array */
    uint32_t postings_count;
} __attribute__((packed)) Term;

/* posting: document id (user-provided id) */
typedef struct {
    uint32_t doc_id;
} __attribute__((packed)) Posting;

/* runtime mmap view */
typedef struct {
    const IndexHeader *hdr;
    const Term *terms;
    const Posting *postings;
    const void *base;
    size_t mapped_size;
} InvertedIndex;

/* API */
KOTOBA_API uint32_t fnv1a(const void *data, size_t len);
KOTOBA_API size_t utf8_char_len(uint8_t c);

/* build: texts[] and doc_ids[] arrays */
KOTOBA_API int index_build_from_pairs(const char *out_path, const char **texts, const uint32_t *doc_ids, size_t count);
/* load/unload */
KOTOBA_API int index_load(const char *path, InvertedIndex *idx);
KOTOBA_API void index_unload(InvertedIndex *idx);

/* lookup a gram hash -> Term* (binary search) */
KOTOBA_API const Term *index_find_term(const InvertedIndex *idx, uint32_t hash);

/* copy postings into out_ids (out_cap must be >= postings_count) */
KOTOBA_API size_t index_term_postings(const InvertedIndex *idx, const Term *t, uint32_t *out_ids, size_t out_cap);

/* helpers for query side: generate unigram+bigram hashes from a UTF-8 query */
/* optimized: fills a preallocated array, no malloc/free interno */
KOTOBA_API size_t query_1_2_gram_hashes(const char *q, uint32_t *out_hashes, size_t max);
