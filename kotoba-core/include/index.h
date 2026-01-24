#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define IDX_MAGIC 0x494E5658u  // "INVX"
#define IDX_VERSION 2u         // bumped because Posting layout changed

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

/* posting: document id (user-provided id) + two metadata words
   - meta1 / meta2 are interpreted by the caller:
     * kanji/readings index: meta1 = reading_idx, meta2 = 0
     * glosses index:        meta1 = sense_id,   meta2 = gloss_id
*/
typedef struct {
    uint32_t doc_id;
    uint8_t meta1;
    uint8_t meta2;
    uint8_t reserved[2];
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

/* build: texts[] and doc_ids[] arrays
   meta1 and meta2 arrays may be NULL (treated as zeros).
   Example usages:
     - kanji/readings TSV: provide meta1 = reading_idx array, meta2 = NULL
     - glosses TSV:        provide meta1 = sense_id array, meta2 = gloss_id array
*/
KOTOBA_API int index_build_from_pairs(
    const char *out_path,
    const char **texts,
    const uint32_t *doc_ids,
    const uint8_t *meta1,   /* optional, can be NULL */
    const uint8_t *meta2,   /* optional, can be NULL */
    size_t count
);

/* load/unload */
KOTOBA_API int index_load(const char *path, InvertedIndex *idx);
KOTOBA_API void index_unload(InvertedIndex *idx);

/* lookup a gram hash -> Term* (binary search) */
KOTOBA_API const Term *index_find_term(const InvertedIndex *idx, uint32_t hash);

/* copy postings into out_postings (out_cap must be >= postings_count) */
KOTOBA_API size_t index_term_postings(const InvertedIndex *idx, const Term *t, Posting *out_postings, size_t out_cap);


KOTOBA_API size_t index_intersect_hashes(
    const InvertedIndex *idx,
    const uint32_t *hashes,
    size_t hcount,
    uint32_t *out,
    size_t out_cap
);

typedef struct {
    const Posting *p;   /* puntero directo al posting en el índice */
} PostingRef;


KOTOBA_API size_t index_intersect_postings(
    const InvertedIndex *idx,
    const uint32_t *hashes,
    size_t hcount,
    PostingRef *out,
    size_t out_cap
);


/* ---------- bigram/unigram generation (UTF-8 aware) ---------- */
/* callback type */
typedef void (*gram_cb)(const uint8_t *p, size_t len, void *ud);

/* generate 1-grams (each codepoint) and 2-grams (prev+curr) */
void utf8_grams_cb(const char *s, gram_cb cb, void *ud)
{
    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *prev = NULL;
    size_t prev_len = 0;

    /* generate unigrams and bigrams in a single pass */
    while (*p)
    {
        size_t len = utf8_char_len(*p);

        /* unigram: current character */
        cb(p, len, ud);

        /* bigram: previous + current character */
        if (prev)
        {
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
/* capture callback ud */
struct QGramUD {
    uint32_t *out;
    size_t max;
    size_t count;
};

static void query_capture_cb(const uint8_t *p, size_t len, void *ud)
{
    struct QGramUD *q = ud;
    if (q->count >= q->max)
        return;
    q->out[q->count++] = fnv1a(p, len);
}
size_t query_gram_hashes(const char *q, uint32_t *out_hashes, size_t max)
{
    if (!q || !out_hashes || max == 0)
        return 0;

    struct QGramUD ud = {
        .out = out_hashes,
        .max = max,
        .count = 0
    };

    utf8_grams_cb(q, query_capture_cb, &ud);
    return ud.count;
}
