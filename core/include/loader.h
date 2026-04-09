#ifndef KOTOBA_LOADER_H
#define KOTOBA_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "types.h"
#include "kotoba.h"

#include <string.h>

/* =========================
   HASH INDEX
   ========================= */

#define RH_STORE_HASH 1
#define RH_MAX_CAPACITY 262144u /* mirror de CardType.h RECALL_OFFSET (debe ser potencia de 2) */

typedef struct {
    uint32_t ent_seq;
    uint32_t index;
#if RH_STORE_HASH
    uint32_t hash;
#endif
} rh_entry;

/* HEADER REAL DEL .idx */
typedef struct {
    uint32_t magic;
    uint32_t capacity;
    uint32_t count;
    uint16_t version;
    uint16_t type;
} rh_header;

/* =========================
   MMAP FILE
   ========================= */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

typedef struct
{
    void   *base;
    size_t  size;

#ifdef _WIN32
    HANDLE file;
    HANDLE mapping;
#else
    int fd;
#endif
} kotoba_file;

/* =========================
   DICT
   ========================= */

typedef struct
{
    kotoba_file bin;
    kotoba_file idx;

    const kotoba_bin_header *bin_header;
    const entry_bin *entries;

    const rh_header *hash_header;
    const rh_entry  *hash_entries;

} kotoba_dict;

/* =========================
   API
   ========================= */

KOTOBA_API int kotoba_file_open(kotoba_file *f, const char *path);
KOTOBA_API void kotoba_file_close(kotoba_file *f);

KOTOBA_API int kotoba_dict_open(kotoba_dict *d,
                        const char *bin_path,
                        const char *idx_path);

KOTOBA_API void kotoba_dict_close(kotoba_dict *d);

KOTOBA_API const entry_bin*
kotoba_dict_get_entry(const kotoba_dict *d, uint32_t i);

KOTOBA_API const entry_bin*
kotoba_dict_get_entry_by_entseq(const kotoba_dict *d, uint32_t ent_seq);

#ifdef __cplusplus
}
#endif

#endif