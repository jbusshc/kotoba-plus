#ifndef KOTOBA_DAT_WRITER_H
#define KOTOBA_DAT_WRITER_H

#include "../../kotoba.h"

typedef struct
{
    FILE *dat_file;
    FILE *offsets_file;

} kotoba_dat_writer;

KOTOBA_API int kotoba_dat_writer_open(kotoba_dat_writer *w,
                               const char *dat_path,
                               const char *offsets_path);
KOTOBA_API int kotoba_dat_writer_write(kotoba_dat_writer *w,
                                const int32_t *base,
                                const int32_t *check,
                                const uint32_t *value,
                                size_t size);

KOTOBA_API void kotoba_dat_writer_close(kotoba_dat_writer *w);

#endif /* KOTOBA_DAT_WRITER_H */