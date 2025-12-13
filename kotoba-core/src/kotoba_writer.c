#include "kotoba_writer.h"
#include "entry_builder.h"
#include <stdlib.h>

int kotoba_writer_open(kotoba_writer *w, const char *path)
{
    w->fp = fopen(path, "wb");
    if (!w->fp) return 0;

    kotoba_file_header h = {
        .magic = KOTOBA_MAGIC,
        .version = KOTOBA_VERSION,
        .entry_count = 0
    };

    fwrite(&h, sizeof(h), 1, w->fp);

    w->index_pos = ftell(w->fp);
    w->entry_count = 0;
    return 1;
}

int kotoba_writer_write_entry(kotoba_writer *w, const entry *e)
{
    binbuf buf;
    build_entry_bin(e, &buf);

    entry_index idx = { .offset = ftell(w->fp) };
    fwrite(&idx, sizeof(idx), 1, w->fp);

    fwrite(buf.data, buf.size, 1, w->fp);
    binbuf_free(&buf);

    w->entry_count++;
    return 1;
}

int kotoba_writer_close(kotoba_writer *w)
{
    fseek(w->fp, 0, SEEK_SET);

    kotoba_file_header h = {
        .magic = KOTOBA_MAGIC,
        .version = KOTOBA_VERSION,
        .entry_count = w->entry_count
    };

    fwrite(&h, sizeof(h), 1, w->fp);
    fclose(w->fp);
    return 1;
}
