#include "kotoba/dat/writer.h"
#include "kotoba/dat/dat.h"
#include <string.h>

static void write_u32(FILE *f, uint32_t v)
{
    fwrite(&v, 4, 1, f);
}

static void write_u16(FILE *f, uint16_t v)
{
    fwrite(&v, 2, 1, f);
}

int kotoba_dat_writer_open(kotoba_dat_writer *w,
                           const char *path)
{
    memset(w, 0, sizeof(*w));

    w->f = fopen(path, "wb");
    if (!w->f)
        return 0;

    return 1;
}

int kotoba_dat_writer_write(kotoba_dat_writer *w,
                            const int32_t *base,
                            const int32_t *check,
                            const int32_t *value,
                            uint32_t node_count)
{
    kotoba_dat_header h = {0};

    h.magic      = KOTOBA_DAT_MAGIC;
    h.version    = KOTOBA_DAT_VERSION;
    h.node_count = node_count;

    fwrite(&h, sizeof(h), 1, w->f);

    fwrite(base,  sizeof(int32_t), node_count, w->f);
    fwrite(check, sizeof(int32_t), node_count, w->f);
    fwrite(value, sizeof(int32_t), node_count, w->f);

    w->node_count = node_count;
    return 1;
}

void kotoba_dat_writer_close(kotoba_dat_writer *w)
{
    if (w->f)
        fclose(w->f);
    memset(w, 0, sizeof(*w));
}
