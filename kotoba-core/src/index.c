// index.c
#define _POSIX_C_SOURCE 200809L
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

/* ---------- utilities ---------- */

uint32_t fnv1a(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; ++i)
    {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

size_t utf8_char_len(uint8_t c)
{
    if ((c & 0x80) == 0x00)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return 1;
}

/* append helper */
static void *append_bytes(void *buf, size_t *cap, size_t *len, const void *data, size_t dlen)
{
    if (*len + dlen > *cap)
    {
        size_t newcap = (*cap == 0) ? 4096 : (*cap * 2);
        while (newcap < *len + dlen)
            newcap *= 2;
        void *n = realloc(buf, newcap);
        if (!n)
        {
            perror("realloc");
            exit(1);
        }
        buf = n;
        *cap = newcap;
    }
    memcpy((uint8_t *)buf + *len, data, dlen);
    *len += dlen;
    return buf;
}

/* ---------- bigram/unigram generation (UTF-8 aware) ---------- */
/* callback type */
typedef void (*gram_cb)(const uint8_t *p, size_t len, void *ud);

/* generate 1-grams (each codepoint) and 2-grams (prev+curr) */
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

/* ---------- builder ---------- */

typedef struct
{
    uint32_t hash;
    uint32_t doc_id;
} Pair;

static int cmp_pair(const void *a, const void *b)
{
    const Pair *x = a, *y = b;
    if (x->hash != y->hash)
        return (x->hash < y->hash) ? -1 : 1;
    if (x->doc_id != y->doc_id)
        return (x->doc_id < y->doc_id) ? -1 : 1;
    return 0;
}

/* build index: texts[i] corresponds to doc_ids[i] */
int index_build_from_pairs(const char *out_path, const char **texts, const uint32_t *doc_ids, size_t count)
{
    Pair *pairs = NULL;
    size_t pcap = 0, pcount = 0;

    /* generator callback that appends Pair(hash, doc_id) */
    void capture_cb(const uint8_t *p, size_t len, void *ud)
    {
        uint32_t doc = (uint32_t)(uintptr_t)ud;
        uint32_t h = fnv1a(p, len);
        if (pcount == pcap)
        {
            pcap = pcap ? pcap * 2 : 65536;
            pairs = realloc(pairs, pcap * sizeof(Pair));
            if (!pairs)
            {
                perror("realloc");
                exit(1);
            }
        }
        pairs[pcount].hash = h;
        pairs[pcount].doc_id = doc;
        pcount++;
    }

    /* generate pairs for all texts */
    for (size_t i = 0; i < count; ++i)
    {
        const char *txt = texts[i];
        uint32_t doc = doc_ids[i];
        if (!txt)
            continue;
        /* we need a wrapper to pass doc to callback, use a small lambda-like function pointer with ud = doc cast */
        /* but utf8_1_2_grams_cb accepts ud, so pass doc as pointer-sized integer */
        utf8_1_2_grams_cb(txt, capture_cb, (void *)(uintptr_t)doc);
    }

    if (pcount == 0)
    {
        fprintf(stderr, "index_build: no grams generated\n");
        free(pairs);
        return -1;
    }

    /* sort pairs by (hash, doc_id) */
    qsort(pairs, pcount, sizeof(Pair), cmp_pair);

    /* count distinct terms */
    size_t term_count = 0;
    for (size_t i = 0; i < pcount;)
    {
        size_t j = i + 1;
        while (j < pcount && pairs[j].hash == pairs[i].hash)
            j++;
        term_count++;
        i = j;
    }

    Term *terms = malloc(term_count * sizeof(Term));
    Posting *postings = malloc(pcount * sizeof(Posting)); /* upper bound */
    if (!terms || !postings)
    {
        perror("malloc");
        exit(1);
    }

    size_t ti = 0, pi = 0;
    for (size_t i = 0; i < pcount;)
    {
        size_t j = i;
        uint32_t last_doc = 0xFFFFFFFFu;
        size_t group_start = pi;
        while (j < pcount && pairs[j].hash == pairs[i].hash)
        {
            uint32_t doc = pairs[j].doc_id;
            if (doc != last_doc)
            {
                postings[pi++].doc_id = doc;
                last_doc = doc;
            }
            j++;
        }
        terms[ti].gram_hash = pairs[i].hash;
        terms[ti].postings_offset = (uint32_t)group_start;
        terms[ti].postings_count = (uint32_t)(pi - group_start);
        ti++;
        i = j;
    }

    /* write out */
    FILE *out = fopen(out_path, "wb");
    if (!out)
    {
        perror("fopen");
        free(pairs);
        free(terms);
        free(postings);
        return -1;
    }
    IndexHeader hdr = {IDX_MAGIC, IDX_VERSION, (uint32_t)ti, (uint32_t)pi};
    fwrite(&hdr, sizeof(hdr), 1, out);
    fwrite(terms, sizeof(Term), ti, out);
    fwrite(postings, sizeof(Posting), pi, out);
    fclose(out);

    free(pairs);
    free(terms);
    free(postings);
    return 0;
}

/* ---------- mmap loader ---------- */
/* ---------- mmap loader cross-platform ---------- */

#ifdef _WIN32
#include <windows.h>
#endif

int index_load(const char *path, InvertedIndex *idx)
{
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateFile failed\n");
        return -1;
    }

    LARGE_INTEGER size_li;
    if (!GetFileSizeEx(hFile, &size_li))
    {
        CloseHandle(hFile);
        return -1;
    }
    size_t size = (size_t)size_li.QuadPart;

    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap)
    {
        CloseHandle(hFile);
        return -1;
    }

    void *base = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMap); /* we can close mapping handle after mapping */
    CloseHandle(hFile);

    if (!base)
    {
        fprintf(stderr, "MapViewOfFile failed\n");
        return -1;
    }

#else
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }
    off_t size = lseek(fd, 0, SEEK_END);
    if (size <= 0)
    {
        close(fd);
        return -1;
    }
    void *base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (base == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }
    size_t size_posix = (size_t)size;
    size = size_posix;
#endif

    const IndexHeader *hdr = (const IndexHeader *)base;
    if (hdr->magic != IDX_MAGIC)
    {
#ifdef _WIN32
        UnmapViewOfFile(base);
#else
        munmap(base, size);
#endif
        return -1;
    }

    idx->hdr = hdr;
    idx->terms = (const Term *)(hdr + 1);
    idx->postings = (const Posting *)(idx->terms + hdr->term_count);
    idx->base = base;
    idx->mapped_size = size;
    return 0;
}

void index_unload(InvertedIndex *idx)
{
    if (!idx || !idx->base)
        return;
#ifdef _WIN32
    UnmapViewOfFile(idx->base);
#else
    munmap((void *)idx->base, idx->mapped_size);
#endif
    idx->base = NULL;
}

const Term *index_find_term(const InvertedIndex *idx, uint32_t hash)
{
    if (!idx || !idx->hdr)
        return NULL;
    uint32_t lo = 0, hi = idx->hdr->term_count;
    while (lo < hi)
    {
        uint32_t mid = (lo + hi) >> 1;
        uint32_t h = idx->terms[mid].gram_hash;
        if (h == hash)
            return &idx->terms[mid];
        if (h < hash)
            lo = mid + 1;
        else
            hi = mid;
    }
    return NULL;
}

size_t index_term_postings(const InvertedIndex *idx, const Term *t, uint32_t *out_ids, size_t out_cap)
{
    if (!t)
        return 0;
    size_t n = t->postings_count;
    if (out_cap < n)
        return 0;
    for (size_t i = 0; i < n; ++i)
        out_ids[i] = idx->postings[t->postings_offset + i].doc_id;
    return n;
}

/* ---------- query helper: generate 1+2 gram hashes ---------- */


size_t query_1_2_gram_hashes(const char *q, uint32_t *out_hashes, size_t max)
{
    if (!q || !out_hashes || max == 0) return 0;

    const uint8_t *p = (const uint8_t *)q;
    const uint8_t *prev = NULL;
    size_t prev_len = 0;
    size_t count = 0;

    while (*p && count < max)
    {
        size_t len = utf8_char_len(*p);

        /* unigram */
        out_hashes[count++] = fnv1a(p, len);
        if (count >= max) break;

        /* bigram */
        if (prev)
        {
            uint8_t tmp[8];
            memcpy(tmp, prev, prev_len);
            memcpy(tmp + prev_len, p, len);
            out_hashes[count++] = fnv1a(tmp, prev_len + len);
        }

        prev = p;
        prev_len = len;
        p += len;
    }

    return count;
}

