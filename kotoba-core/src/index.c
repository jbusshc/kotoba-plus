// index.c
#define _POSIX_C_SOURCE 200809L
#include "index.h"

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

/* ---------- builder ---------- */

typedef struct
{
    uint32_t hash;
    uint32_t doc_id;
    uint8_t meta1;
    uint8_t meta2;
} Pair;

static int cmp_pair(const void *a, const void *b)
{
    const Pair *x = a, *y = b;
    if (x->hash != y->hash)
        return (x->hash < y->hash) ? -1 : 1;
    if (x->doc_id != y->doc_id)
        return (x->doc_id < y->doc_id) ? -1 : 1;
    if (x->meta1 != y->meta1)
        return (x->meta1 < y->meta1) ? -1 : 1;
    if (x->meta2 != y->meta2)
        return (x->meta2 < y->meta2) ? -1 : 1;
    return 0;
}

#define META_NONE UINT8_MAX

/* build index: texts[i] corresponds to doc_ids[i], optional meta1/meta2 arrays */

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

    /* optional: check version compatibility */
    if (hdr->version != IDX_VERSION)
    {
        fprintf(stderr, "index_load: unexpected version %u (expected %u)\n", hdr->version, IDX_VERSION);
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

/* copy postings into out_postings (out_cap must be >= postings_count) */
size_t index_term_postings(const InvertedIndex *idx, const Term *t, Posting *out_postings, size_t out_cap)
{
    if (!t)
        return 0;
    size_t n = t->postings_count;
    if (out_cap < n)
        return 0;
    for (size_t i = 0; i < n; ++i)
    {
        const Posting *p = &idx->postings[t->postings_offset + i];
        out_postings[i] = *p;
    }
    return n;
}

/* Helper struct and callback for query_gram_hashes_mode */

static void query_gram_hash_cb(const uint8_t *p, size_t l, void *u)
{
    struct QGramUD *q = (struct QGramUD *)u;
    if (q->count < q->max)
        q->out[q->count++] = fnv1a(p, l);
}


size_t index_intersect_postings(
    const InvertedIndex *idx,
    const uint32_t *hashes,
    size_t hcount,
    PostingRef *out,
    size_t out_cap)
{
    if (!idx || !hashes || hcount == 0 || !out || out_cap <= 0)
        return 0;

    const Term *terms[128];
    if (hcount > 128)
        return 0;

    /* lookup all terms */
    for (size_t i = 0; i < hcount; ++i)
    {
        terms[i] = index_find_term(idx, hashes[i]);
        if (!terms[i])
            return 0; /* algún gram no existe → intersección vacía */
    }

    /* find smallest postings list */
    size_t min_i = 0;
    for (size_t i = 1; i < hcount; ++i)
    {
        if (terms[i]->postings_count < terms[min_i]->postings_count)
            min_i = i;
    }

    const Term *base = terms[min_i];
    const Posting *base_post = idx->postings + base->postings_offset;

    size_t written = 0;
    uint32_t last_written_doc = 0xFFFFFFFFu;

    /* iterate base postings */
    for (size_t bi = 0; bi < base->postings_count; ++bi)
    {
        const Posting *bp = &base_post[bi];
        uint32_t doc = bp->doc_id;

        if (doc == last_written_doc)
            continue; /* evita duplicados */

        int ok = 1;

        /* check presence in all other postings */
        for (size_t ti = 0; ti < hcount; ++ti)
        {
            if (ti == min_i)
                continue;

            const Term *t = terms[ti];
            const Posting *p = idx->postings + t->postings_offset;

            /* binary search by doc_id */
            uint32_t lo = 0, hi = t->postings_count;
            while (lo < hi)
            {
                uint32_t mid = (lo + hi) >> 1;
                uint32_t v = p[mid].doc_id;

                if (v == doc)
                {
                    lo = mid;
                    break;
                }
                if (v < doc)
                    lo = mid + 1;
                else
                    hi = mid;
            }

            if (lo >= t->postings_count || p[lo].doc_id != doc)
            {
                ok = 0;
                break;
            }
        }

        if (ok)
        {
            if (written >= out_cap)
                break;

            out[written++].p = bp; /* 🔑 aquí está la magia */
            last_written_doc = doc;
        }
    }

    return written;
}


size_t utf8_strlen(const char *s)
{
    size_t len = 0;
    const uint8_t *p = (const uint8_t *)s;
    while (*p)
    {
        p += utf8_char_len(*p);
        len++;
    }
    return len;
}

void utf8_ngrams_cb(const char *s, int n, gram_cb cb, void *ud)
{
    if (n <= 0)
        return;

    const uint8_t *p = (const uint8_t *)s;
    const uint8_t *win[4];
    size_t win_len[4];
    int w = 0;

    while (*p)
    {
        size_t len = utf8_char_len(*p);
        win[w] = p;
        win_len[w] = len;
        w++;

        if (w == n)
        {
            uint8_t buf[16];
            size_t off = 0;
            for (int i = 0; i < n; ++i)
            {
                memcpy(buf + off, win[i], win_len[i]);
                off += win_len[i];
            }
            cb(buf, off, ud);

            /* slide window */
            for (int i = 1; i < n; ++i)
            {
                win[i - 1] = win[i];
                win_len[i - 1] = win_len[i];
            }
            w--;
        }

        p += len;
    }
}

int index_build_from_pairs(
    const char *out_path,
    const char **texts,
    const uint32_t *doc_ids,
    const uint8_t *meta1,
    const uint8_t *meta2,
    size_t count,
    GramMode gram_mode)
{
    Pair *pairs = NULL; size_t pcap = 0, pcount = 0;

    struct GramUD { uint32_t doc; uint8_t m1; uint8_t m2; };
    void capture_cb(const uint8_t *p, size_t len, void *ud)
    {
        struct GramUD *g = (struct GramUD*)ud;
        uint32_t h = fnv1a(p, len);
        if (pcount == pcap) { pcap = pcap ? pcap*2 : 65536; pairs = realloc(pairs, pcap*sizeof(Pair)); if(!pairs){perror("realloc");exit(1);} }
        pairs[pcount].hash = h; pairs[pcount].doc_id = g->doc; pairs[pcount].meta1 = g->m1; pairs[pcount].meta2 = g->m2; pcount++;
    }

    for(size_t i=0;i<count;++i)
    {
        const char *txt = texts[i]; if(!txt) continue;
        struct GramUD ud = { doc_ids?doc_ids[i]:0, meta1?meta1[i]:META_NONE, meta2?meta2[i]:META_NONE };
        size_t len = utf8_strlen(txt);

        if(gram_mode==GRAM_JP_ALL)
        {
            // siempre generar uni+bi+tri grams
            utf8_ngrams_cb(txt,1,capture_cb,&ud);
            utf8_ngrams_cb(txt,2,capture_cb,&ud);
            utf8_ngrams_cb(txt,3,capture_cb,&ud);
        }
        else if(gram_mode==GRAM_GLOSS_AUTO)
        {
            // auto según longitud, como antes
            if(len==1) utf8_ngrams_cb(txt,1,capture_cb,&ud);
            else if(len==2) utf8_ngrams_cb(txt,2,capture_cb,&ud);
            else utf8_ngrams_cb(txt,3,capture_cb,&ud);
        }
        else
        {
            utf8_ngrams_cb(txt, gram_mode, capture_cb,&ud);
        }
    }

    if(pcount==0){fprintf(stderr,"index_build: no grams generated\n");free(pairs);return -1;}

    /* --- sort y dedupe --- */
    int cmp_pair(const void *a,const void *b)
    {
        const Pair *x=a,*y=b;
        if(x->hash!=y->hash) return x->hash<y->hash?-1:1;
        if(x->doc_id!=y->doc_id) return x->doc_id<y->doc_id?-1:1;
        if(x->meta1!=y->meta1) return x->meta1<y->meta1?-1:1;
        if(x->meta2!=y->meta2) return x->meta2<y->meta2?-1:1;
        return 0;
    }
    qsort(pairs,pcount,sizeof(Pair),cmp_pair);

    /* --- generar terms y postings --- */
    size_t term_count=0; for(size_t i=0;i<pcount;)
    {
        size_t j=i+1; while(j<pcount && pairs[j].hash==pairs[i].hash) j++; term_count++; i=j;
    }

    Term *terms = malloc(term_count*sizeof(Term));
    Posting *postings = malloc(pcount*sizeof(Posting));
    if(!terms||!postings){perror("malloc");exit(1);}

    size_t ti=0,pi=0;
    for(size_t i=0;i<pcount;)
    {
        size_t j=i; uint32_t last_doc=0xFFFFFFFFu; uint8_t last_m1=0xFF,last_m2=0xFF;
        size_t group_start=pi;
        while(j<pcount && pairs[j].hash==pairs[i].hash)
        {
            uint32_t doc=pairs[j].doc_id; uint8_t m1=pairs[j].meta1,m2=pairs[j].meta2;
            if(!(doc==last_doc && m1==last_m1 && m2==last_m2))
            {
                postings[pi].doc_id=doc; postings[pi].meta1=m1; postings[pi].meta2=m2; pi++;
                last_doc=doc; last_m1=m1; last_m2=m2;
            }
            j++;
        }
        terms[ti].gram_hash=pairs[i].hash;
        terms[ti].postings_offset=(uint32_t)group_start;
        terms[ti].postings_count=(uint32_t)(pi-group_start);
        ti++; i=j;
    }

    FILE *out=fopen(out_path,"wb");
    if(!out){perror("fopen");free(pairs);free(terms);free(postings);return -1;}
    IndexHeader hdr={IDX_MAGIC,IDX_VERSION,(uint32_t)ti,(uint32_t)pi};
    fwrite(&hdr,sizeof(hdr),1,out);
    fwrite(terms,sizeof(Term),ti,out);
    fwrite(postings,sizeof(Posting),pi,out);
    fclose(out);

    free(pairs); free(terms); free(postings);
    return 0;
}

size_t query_gram_hashes_mode(const char *q, GramMode mode, uint32_t *out_hashes, size_t max)
{
    if(!q||!out_hashes||max==0) return 0;
    struct QGramUD { uint32_t *out; size_t max,count; } ud = { out_hashes,max,0 };
    size_t len=utf8_strlen(q);

    void cb(const uint8_t *p, size_t l, void *u){ struct QGramUD *q=(struct QGramUD*)u; if(q->count<q->max) q->out[q->count++]=fnv1a(p,l); }

    if(mode==GRAM_GLOSS_AUTO)
    {
        if(len==1) utf8_ngrams_cb(q,1,cb,&ud);
        else if(len==2) utf8_ngrams_cb(q,2,cb,&ud);
        else utf8_ngrams_cb(q,3,cb,&ud);
    }
    else if(mode==GRAM_JP_ALL)
    {
        // uni + bi + tri
        utf8_ngrams_cb(q,1,cb,&ud);
        utf8_ngrams_cb(q,2,cb,&ud);
        utf8_ngrams_cb(q,3,cb,&ud);
    }
    else
    {
        utf8_ngrams_cb(q,mode,cb,&ud);
    }

    return ud.count;
}
