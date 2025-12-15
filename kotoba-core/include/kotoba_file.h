#ifndef KOTOBA_FILE_H
#define KOTOBA_FILE_H

#include <stdint.h>
#include <stddef.h>
#include "kotoba_types.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

typedef struct {
    const uint8_t *base;
    size_t         size;
    kotoba_bin_header header;

#ifdef _WIN32
    void *hFile;
    void *hMap;
#else
    int fd;
#endif
} kotoba_file;

int kotoba_file_open(kotoba_file *kf, const char *path);
void kotoba_file_close(kotoba_file *kf);

#endif
