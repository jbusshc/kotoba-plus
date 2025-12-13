#ifndef KOTOBA_WRITER_H
#define KOTOBA_WRITER_H

#include "kotoba_types.h"
#include <stdio.h>

typedef struct {
    FILE *fp;
    uint32_t entry_count;
    long index_pos;
} kotoba_writer;

int kotoba_writer_open(kotoba_writer *w, const char *path);
int kotoba_writer_write_entry(kotoba_writer *w, const entry *e);
int kotoba_writer_close(kotoba_writer *w);

#endif
