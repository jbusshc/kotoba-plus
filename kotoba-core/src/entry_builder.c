#include "entry_builder.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void grow(binbuf *b, uint32_t need) {
    if (b->size + need <= b->capacity) return;
    b->capacity = (b->capacity + need) * 2;
    b->data = realloc(b->data, b->capacity);
}

void binbuf_init(binbuf *b) {
    b->data = NULL;
    b->size = 0;
    b->capacity = 0;
}

void binbuf_free(binbuf *b) {
    free(b->data);
}

void binbuf_write(binbuf *b, const void *src, uint32_t n) {
    grow(b, n);
    memcpy(b->data + b->size, src, n);
    b->size += n;
}

uint32_t binbuf_tell(const binbuf *b) {
    return b->size;
}

void build_entry_bin(const entry *e, binbuf *out)
{
    binbuf_init(out);

    /* ================= entry header ================= */
    entry_bin hdr = {0};
    hdr.ent_seq  = e->ent_seq;
    hdr.k_count  = e->k_elements_count;
    hdr.r_count  = e->r_elements_count;
    hdr.s_count  = e->senses_count;
    hdr.priority = e->priority;

    uint32_t hdr_pos = binbuf_tell(out);
    binbuf_write(out, &hdr, sizeof(hdr));

    /* ================= k_ele ================= */
    hdr.k_off = binbuf_tell(out);

    uint32_t *k_patch[1024];
    int k_patch_count = 0;

    for (int i = 0; i < e->k_elements_count; i++) {
        const k_ele *k = &e->k_elements[i];

        k_patch[k_patch_count++] =
            (uint32_t *)(out->data + write_off_len8(out, strlen(k->keb)));

        uint8_t ic = k->ke_inf_count;
        uint8_t pc = k->ke_pri_count;
        binbuf_write(out, &ic, 1);
        binbuf_write(out, &pc, 1);

        for (int j = 0; j < ic; j++)
            k_patch[k_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(k->ke_inf[j])));

        for (int j = 0; j < pc; j++)
            k_patch[k_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(k->ke_pri[j])));
    }

    /* ================= r_ele ================= */
    hdr.r_off = binbuf_tell(out);

    uint32_t *r_patch[2048];
    int r_patch_count = 0;

    for (int i = 0; i < e->r_elements_count; i++) {
        const r_ele *r = &e->r_elements[i];

        r_patch[r_patch_count++] =
            (uint32_t *)(out->data + write_off_len8(out, strlen(r->reb)));

        uint8_t nok = r->re_nokanji;
        uint8_t rc  = r->re_restr_count;
        uint8_t ic  = r->re_inf_count;
        uint8_t pc  = r->re_pri_count;

        binbuf_write(out, &nok, 1);
        binbuf_write(out, &rc,  1);
        binbuf_write(out, &ic,  1);
        binbuf_write(out, &pc,  1);

        for (int j = 0; j < rc; j++)
            r_patch[r_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(r->re_restr[j])));

        for (int j = 0; j < ic; j++)
            r_patch[r_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(r->re_inf[j])));

        for (int j = 0; j < pc; j++)
            r_patch[r_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(r->re_pri[j])));
    }

    /* ================= sense ================= */
    hdr.s_off = binbuf_tell(out);

    uint32_t *s_patch[8192];
    int s_patch_count = 0;

    for (int i = 0; i < e->senses_count; i++) {
        const sense *s = &e->senses[i];

        uint8_t counts[12] = {
            s->stagk_count, s->stagr_count, s->pos_count,   s->xref_count,
            s->ant_count,   s->field_count, s->misc_count,  s->s_inf_count,
            s->lsource_count, s->dial_count, s->gloss_count, s->examples_count
        };

        binbuf_write(out, counts, 12);
        binbuf_write(out, &s->lang, 1);

        for (int j = 0; j < s->stagk_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->stagk[j])));

        for (int j = 0; j < s->stagr_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->stagr[j])));

        for (int j = 0; j < s->pos_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->pos[j])));

        for (int j = 0; j < s->xref_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->xref[j])));

        for (int j = 0; j < s->ant_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->ant[j])));

        for (int j = 0; j < s->field_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->field[j])));

        for (int j = 0; j < s->misc_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->misc[j])));

        for (int j = 0; j < s->s_inf_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len16(out, strlen(s->s_inf[j])));

        for (int j = 0; j < s->lsource_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->lsource[j])));

        for (int j = 0; j < s->dial_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(s->dial[j])));

        for (int j = 0; j < s->gloss_count; j++)
            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len16(out, strlen(s->gloss[j])));

        /* examples */
        for (int j = 0; j < s->examples_count; j++) {
            const example *ex = &s->examples[j];

            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len8(out, strlen(ex->ex_srce)));

            s_patch[s_patch_count++] =
                (uint32_t *)(out->data + write_off_len16(out, strlen(ex->ex_text)));

            uint8_t sc = ex->ex_sent_count;
            binbuf_write(out, &sc, 1);

            for (int k = 0; k < sc; k++)
                s_patch[s_patch_count++] =
                    (uint32_t *)(out->data + write_off_len16(out, strlen(ex->ex_sent[k])));
        }
    }

    /* ================= string buffer ================= */
    hdr.buffer_off = binbuf_tell(out);

    #define EMIT_STR(ptr) do { \
        uint32_t off = binbuf_tell(out) - hdr.buffer_off; \
        patch_off(out, (uint8_t*)ptr - out->data, off); \
        binbuf_write(out, ptr, strlen(ptr)); \
    } while(0)

    /* emitir strings en el MISMO orden */
    for (int i = 0; i < k_patch_count; i++) EMIT_STR((char*)k_patch[i]);
    for (int i = 0; i < r_patch_count; i++) EMIT_STR((char*)r_patch[i]);
    for (int i = 0; i < s_patch_count; i++) EMIT_STR((char*)s_patch[i]);

    hdr.buffer_size = binbuf_tell(out) - hdr.buffer_off;
    hdr.total_size  = binbuf_tell(out);

    memcpy(out->data + hdr_pos, &hdr, sizeof(hdr));
}
