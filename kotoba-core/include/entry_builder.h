#ifndef ENTRY_BUILDER_H
#define ENTRY_BUILDER_H

#include "kotoba_types.h"
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
} binbuf;

void binbuf_init(binbuf *b);
void binbuf_free(binbuf *b);
void binbuf_write(binbuf *b, const void *src, uint32_t n);
uint32_t binbuf_tell(const binbuf *b);

void build_entry_bin(const entry *src, binbuf *out);

#endif
