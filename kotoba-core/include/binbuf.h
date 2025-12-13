/* binbuf.h */
#ifndef BINBUF_H
#define BINBUF_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t cap;
} binbuf;

static inline void binbuf_init(binbuf *b) {
    b->data = NULL;
    b->size = 0;
    b->cap  = 0;
}

static inline uint32_t binbuf_tell(const binbuf *b) {
    return b->size;
}

static inline void binbuf_reserve(binbuf *b, uint32_t extra) {
    if (b->size + extra <= b->cap) return;
    uint32_t newcap = b->cap ? b->cap * 2 : 1024;
    while (newcap < b->size + extra)
        newcap *= 2;
    b->data = realloc(b->data, newcap);
    b->cap = newcap;
}

static inline void binbuf_write(binbuf *b, const void *src, uint32_t n) {
    binbuf_reserve(b, n);
    memcpy(b->data + b->size, src, n);
    b->size += n;
}

static inline uint32_t write_off_len8(binbuf *b, uint8_t len) {
    uint32_t pos = binbuf_tell(b);
    uint32_t off = 0;
    binbuf_write(b, &off, 4);
    binbuf_write(b, &len, 1);
    return pos;
}

static inline uint32_t write_off_len16(binbuf *b, uint16_t len) {
    uint32_t pos = binbuf_tell(b);
    uint32_t off = 0;
    binbuf_write(b, &off, 4);
    binbuf_write(b, &len, 2);
    return pos;
}

static inline void patch_off(binbuf *b, uint32_t pos, uint32_t off) {
    memcpy(b->data + pos, &off, 4);
}


#endif
