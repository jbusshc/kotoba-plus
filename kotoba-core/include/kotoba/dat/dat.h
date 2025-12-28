#ifndef KOTOBA_DAT_H
#define KOTOBA_DAT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * TRIE
 * ============================================================ */

#define TRIE_INITIAL_CAP 1024

typedef struct trie_node {
    int child[256];
    uint8_t term;
    uint32_t id_head;
} trie_node;

typedef struct {
    trie_node *nodes;
    uint32_t size;
    uint32_t cap;
} trie_t;

/* ---------------- TRIE API ---------------- */

static void trie_init(trie_t *t)
{
    t->cap = TRIE_INITIAL_CAP;
    t->size = 1;
    t->nodes = (trie_node *)malloc(t->cap * sizeof(trie_node));
    assert(t->nodes);

    for (int i = 0; i < 256; i++)
        t->nodes[0].child[i] = -1;
    t->nodes[0].term = 0;
    t->nodes[0].id_head = 0;
}

static uint32_t trie_new_node(trie_t *t)
{
    if (t->size >= t->cap) {
        t->cap *= 2;
        t->nodes = (trie_node *)realloc(t->nodes, t->cap * sizeof(trie_node));
        assert(t->nodes);
    }

    uint32_t id = t->size++;
    for (int i = 0; i < 256; i++)
        t->nodes[id].child[i] = -1;
    t->nodes[id].term = 0;
    t->nodes[id].id_head = 0;
    return id;
}

/* insertar palabra + ID (permite múltiples IDs) */
static void trie_insert(trie_t *t,
                        const uint8_t *data, size_t len,
                        uint32_t entry_id)
{
    uint32_t node = 0;

    for (size_t i = 0; i < len; i++) {
        uint8_t c = data[i];
        if (t->nodes[node].child[c] == -1)
            t->nodes[node].child[c] = (int)trie_new_node(t);
        node = (uint32_t)t->nodes[node].child[c];
    }

    t->nodes[node].term = 1;

    /* push-front en lista de IDs */
    uint32_t id_node = (uint32_t)t->nodes[node].id_head;
    t->nodes[node].id_head = entry_id | (id_node << 32);
    /* NOTA: el trie solo acumula, el pool real está en DAT */
}

static void trie_free(trie_t *t)
{
    free(t->nodes);
}

/* ============================================================
 * DOUBLE ARRAY TRIE + ID POOL
 * ============================================================ */

#define DAT_ROOT 1
#define DAT_INITIAL_CAP 1024
#define DAT_ID_INITIAL_CAP 1024

typedef struct {
    uint32_t id;
    uint32_t next;
} dat_id_node;

typedef struct {
    int32_t  *base;
    int32_t  *check;
    uint8_t  *term;

    uint32_t *first_child;
    uint32_t *next_sibling;

    uint32_t *id_head;

    dat_id_node *id_pool;
    uint32_t id_pool_size;
    uint32_t id_pool_cap;

    uint32_t cap;
    uint32_t size;
} dat_t;

/* ---------------- DAT helpers ---------------- */

static void dat_init(dat_t *d)
{
    d->cap = DAT_INITIAL_CAP;
    d->size = DAT_ROOT + 1;

    d->base  = (int32_t *)calloc(d->cap, sizeof(int32_t));
    d->check = (int32_t *)calloc(d->cap, sizeof(int32_t));
    d->term  = (uint8_t *)calloc(d->cap, sizeof(uint8_t));

    d->first_child  = (uint32_t *)calloc(d->cap, sizeof(uint32_t));
    d->next_sibling = (uint32_t *)calloc(d->cap, sizeof(uint32_t));
    d->id_head      = (uint32_t *)calloc(d->cap, sizeof(uint32_t));

    d->id_pool_cap = DAT_ID_INITIAL_CAP;
    d->id_pool_size = 1; /* 0 = null */
    d->id_pool = (dat_id_node *)calloc(d->id_pool_cap, sizeof(dat_id_node));

    assert(d->base && d->check && d->term);
    assert(d->first_child && d->next_sibling && d->id_head);
    assert(d->id_pool);
}

static uint32_t dat_id_push(dat_t *d, uint32_t id, uint32_t next)
{
    if (d->id_pool_size >= d->id_pool_cap) {
        d->id_pool_cap *= 2;
        d->id_pool = (dat_id_node *)
            realloc(d->id_pool, d->id_pool_cap * sizeof(dat_id_node));
        assert(d->id_pool);
    }

    uint32_t idx = d->id_pool_size++;
    d->id_pool[idx].id = id;
    d->id_pool[idx].next = next;
    return idx;
}

static void dat_ensure(dat_t *d, uint32_t need)
{
    if (need < d->cap)
        return;

    uint32_t new_cap = d->cap;
    while (new_cap <= need)
        new_cap *= 2;

    d->base  = (int32_t *)realloc(d->base,  new_cap * sizeof(int32_t));
    d->check = (int32_t *)realloc(d->check, new_cap * sizeof(int32_t));
    d->term  = (uint8_t *)realloc(d->term,  new_cap * sizeof(uint8_t));

    d->first_child  = (uint32_t *)realloc(d->first_child,  new_cap * sizeof(uint32_t));
    d->next_sibling = (uint32_t *)realloc(d->next_sibling, new_cap * sizeof(uint32_t));
    d->id_head      = (uint32_t *)realloc(d->id_head,      new_cap * sizeof(uint32_t));

    assert(d->base && d->check && d->term);
    assert(d->first_child && d->next_sibling && d->id_head);

    for (uint32_t i = d->cap; i < new_cap; i++) {
        d->base[i] = d->check[i] = 0;
        d->term[i] = 0;
        d->first_child[i] = 0;
        d->next_sibling[i] = 0;
        d->id_head[i] = 0;
    }

    d->cap = new_cap;
}

static int dat_conflict(dat_t *d, int base, uint8_t c)
{
    uint32_t idx = (uint32_t)(base + c);
    return (idx < d->cap && d->check[idx] != 0);
}

static int dat_find_base(dat_t *d, const uint8_t *keys, size_t n)
{
    for (int base = 1;; base++) {
        int ok = 1;
        for (size_t i = 0; i < n; i++) {
            if (dat_conflict(d, base, keys[i])) {
                ok = 0;
                break;
            }
        }
        if (ok)
            return base;
    }
}

/* ============================================================
 * TRIE -> DAT BUILDER
 * ============================================================ */

static void dat_build_rec(dat_t *d, trie_t *t,
                          uint32_t tnode, uint32_t dnode)
{
    uint8_t keys[256];
    size_t nkeys = 0;

    for (int c = 0; c < 256; c++) {
        if (t->nodes[tnode].child[c] != -1)
            keys[nkeys++] = (uint8_t)c;
    }

    uint32_t prev_child = 0;

    if (nkeys > 0) {
        int base = dat_find_base(d, keys, nkeys);
        d->base[dnode] = base;

        for (size_t i = 0; i < nkeys; i++) {
            uint8_t c = keys[i];
            uint32_t next = (uint32_t)(base + c);

            dat_ensure(d, next + 1);
            d->check[next] = (int32_t)dnode;

            if (prev_child == 0)
                d->first_child[dnode] = next;
            else
                d->next_sibling[prev_child] = next;

            prev_child = next;

            if (next >= d->size)
                d->size = next + 1;

            dat_build_rec(d, t,
                (uint32_t)t->nodes[tnode].child[c],
                next);
        }
    }

    if (t->nodes[tnode].term) {
        d->term[dnode] = 1;

        /* copiar lista de IDs */
        uint32_t head = 0;
        for (uint32_t p = t->nodes[tnode].id_head; p != 0; ) {
            uint32_t id = p; /* ID directo desde el trie */
            head = dat_id_push(d, id, head);
            break;
        }
        d->id_head[dnode] = head;
    }
}

static void dat_build_from_trie(dat_t *d, trie_t *t)
{
    dat_init(d);
    dat_build_rec(d, t, 0, DAT_ROOT);
}

/* ============================================================
 * ENUMERACIÓN DE IDS
 * ============================================================ */

static void dat_enum_ids(dat_t *d, uint32_t node,
                         void (*cb)(uint32_t id, void *user),
                         void *user)
{
    for (uint32_t p = d->id_head[node]; p != 0; p = d->id_pool[p].next)
        cb(d->id_pool[p].id, user);
}

static void dat_enum_from(dat_t *d, uint32_t node,
                          void (*cb)(uint32_t id, void *user),
                          void *user)
{
    if (d->term[node])
        dat_enum_ids(d, node, cb, user);

    for (uint32_t c = d->first_child[node]; c != 0; c = d->next_sibling[c])
        dat_enum_from(d, c, cb, user);
}

#endif /* KOTOBA_DAT_H */
