// kotoba_writer.c
#include "kotoba_writer.h"

/* Función principal: escribe un entry_bin completo en el archivo */
uint32_t     write_entry(FILE *fp, const entry *e) {
    uint32_t entry_start = ftell(fp);

    // --- Reservamos espacio para entry_bin ---
    entry_bin eb;
    memset(&eb, 0, sizeof(entry_bin));
    eb.ent_seq = e->ent_seq;
    eb.k_count = (uint8_t)e->k_elements_count;
    eb.r_count = (uint8_t)e->r_elements_count;
    eb.s_count = (uint8_t)e->senses_count;
    eb.priority = e->priority;

    fwrite(&eb, sizeof(entry_bin), 1, fp);

    // --- Escribir k_ele_bin ---
    uint32_t k_start = ftell(fp) - entry_start;
    for(int i=0;i<e->k_elements_count;i++){
        k_ele_bin keb;
        memset(&keb,0,sizeof(k_ele_bin));

        // Escribir keb string
        keb.keb_off = write_string(fp, e->k_elements[i].keb, entry_start);

        // Escribir ke_inf strings y offsets
        uint32_t tmp_offsets[MAX_KE_INF];
        for(int j=0;j<e->k_elements[i].ke_inf_count;j++)
            tmp_offsets[j] = write_string(fp, e->k_elements[i].ke_inf[j], entry_start);
        keb.ke_inf_count = e->k_elements[i].ke_inf_count;
        keb.ke_inf_off_off = write_offsets(fp, tmp_offsets, keb.ke_inf_count, entry_start);

        // Escribir ke_pri strings y offsets
        for(int j=0;j<e->k_elements[i].ke_pri_count;j++)
            tmp_offsets[j] = write_string(fp, e->k_elements[i].ke_pri[j], entry_start);
        keb.ke_pri_count = e->k_elements[i].ke_pri_count;
        keb.ke_pri_off_off = write_offsets(fp, tmp_offsets, keb.ke_pri_count, entry_start);

        fwrite(&keb, sizeof(k_ele_bin), 1, fp);
    }
    eb.k_off = k_start;

    // --- Escribir r_ele_bin ---
    uint32_t r_start = ftell(fp) - entry_start;
    for(int i=0;i<e->r_elements_count;i++){
        r_ele_bin reb;
        memset(&reb,0,sizeof(r_ele_bin));

        reb.reb_off = write_string(fp, e->r_elements[i].reb, entry_start);
        reb.re_nokanji = e->r_elements[i].re_nokanji;

        uint32_t tmp_offsets[MAX_RE_PRI];

        // re_restr
        for(int j=0;j<e->r_elements[i].re_restr_count;j++)
            tmp_offsets[j] = write_string(fp, e->r_elements[i].re_restr[j], entry_start);
        reb.re_restr_count = e->r_elements[i].re_restr_count;
        reb.re_restr_off_off = write_offsets(fp, tmp_offsets, reb.re_restr_count, entry_start);

        // re_inf
        for(int j=0;j<e->r_elements[i].re_inf_count;j++)
            tmp_offsets[j] = write_string(fp, e->r_elements[i].re_inf[j], entry_start);
        reb.re_inf_count = e->r_elements[i].re_inf_count;
        reb.re_inf_off_off = write_offsets(fp, tmp_offsets, reb.re_inf_count, entry_start);

        // re_pri
        for(int j=0;j<e->r_elements[i].re_pri_count;j++)
            tmp_offsets[j] = write_string(fp, e->r_elements[i].re_pri[j], entry_start);
        reb.re_pri_count = e->r_elements[i].re_pri_count;
        reb.re_pri_off_off = write_offsets(fp, tmp_offsets, reb.re_pri_count, entry_start);

        fwrite(&reb, sizeof(r_ele_bin), 1, fp);
    }
    eb.r_off = r_start;

    // --- Escribir senses ---
    uint32_t s_start = ftell(fp) - entry_start;
    for(int i=0;i<e->senses_count;i++){
        sense_bin sb;
        memset(&sb,0,sizeof(sense_bin));
        sb.lang = e->senses[i].lang;

        uint32_t tmp_offsets[MAX_GLOSS]; // temporal para todos los arrays de offsets

        // stagk
        for(int j=0;j<e->senses[i].stagk_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].stagk[j], entry_start);
        sb.stagk_count = e->senses[i].stagk_count;
        sb.stagk_off_off = write_offsets(fp, tmp_offsets, sb.stagk_count, entry_start);

        // stagr
        for(int j=0;j<e->senses[i].stagr_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].stagr[j], entry_start);
        sb.stagr_count = e->senses[i].stagr_count;
        sb.stagr_off_off = write_offsets(fp, tmp_offsets, sb.stagr_count, entry_start);

        // pos
        for(int j=0;j<e->senses[i].pos_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].pos[j], entry_start);
        sb.pos_count = e->senses[i].pos_count;
        sb.pos_off_off = write_offsets(fp, tmp_offsets, sb.pos_count, entry_start);

        // xref
        for(int j=0;j<e->senses[i].xref_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].xref[j], entry_start);
        sb.xref_count = e->senses[i].xref_count;
        sb.xref_off_off = write_offsets(fp, tmp_offsets, sb.xref_count, entry_start);

        // ant
        for(int j=0;j<e->senses[i].ant_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].ant[j], entry_start);
        sb.ant_count = e->senses[i].ant_count;
        sb.ant_off_off = write_offsets(fp, tmp_offsets, sb.ant_count, entry_start);

        // field
        for(int j=0;j<e->senses[i].field_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].field[j], entry_start);
        sb.field_count = e->senses[i].field_count;
        sb.field_off_off = write_offsets(fp, tmp_offsets, sb.field_count, entry_start);

        // misc
        for(int j=0;j<e->senses[i].misc_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].misc[j], entry_start);
        sb.misc_count = e->senses[i].misc_count;
        sb.misc_off_off = write_offsets(fp, tmp_offsets, sb.misc_count, entry_start);

        // s_inf
        for(int j=0;j<e->senses[i].s_inf_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].s_inf[j], entry_start);
        sb.s_inf_count = e->senses[i].s_inf_count;
        sb.s_inf_off_off = write_offsets(fp, tmp_offsets, sb.s_inf_count, entry_start);

        // lsource
        for(int j=0;j<e->senses[i].lsource_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].lsource[j], entry_start);
        sb.lsource_count = e->senses[i].lsource_count;
        sb.lsource_off_off = write_offsets(fp, tmp_offsets, sb.lsource_count, entry_start);

        // dial
        for(int j=0;j<e->senses[i].dial_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].dial[j], entry_start);
        sb.dial_count = e->senses[i].dial_count;
        sb.dial_off_off = write_offsets(fp, tmp_offsets, sb.dial_count, entry_start);

        // gloss
        for(int j=0;j<e->senses[i].gloss_count;j++)
            tmp_offsets[j] = write_string(fp, e->senses[i].gloss[j], entry_start);
        sb.gloss_count = e->senses[i].gloss_count;
        sb.gloss_off_off = write_offsets(fp, tmp_offsets, sb.gloss_count, entry_start);

        // examples
        uint32_t example_offsets[MAX_EXAMPLES];
        for(int j=0;j<e->senses[i].examples_count;j++){
            example_bin ex;
            memset(&ex,0,sizeof(example_bin));
            ex.ex_srce_off = write_string(fp, e->senses[i].examples[j].ex_srce, entry_start);
            ex.ex_text_off = write_string(fp, e->senses[i].examples[j].ex_text, entry_start);

            uint32_t sent_offsets[MAX_EX_SENT];
            for(int k=0;k<e->senses[i].examples[j].ex_sent_count;k++)
                sent_offsets[k] = write_string(fp, e->senses[i].examples[j].ex_sent[k], entry_start);

            ex.ex_sent_count = e->senses[i].examples[j].ex_sent_count;
            ex.ex_sent_off_off = write_offsets(fp, sent_offsets, ex.ex_sent_count, entry_start);

            example_offsets[j] = ftell(fp) - entry_start;
            fwrite(&ex, sizeof(example_bin),1,fp);
        }
        sb.examples_count = e->senses[i].examples_count;
        sb.examples_off_off = write_offsets(fp, example_offsets, sb.examples_count, entry_start);

        fwrite(&sb, sizeof(sense_bin),1,fp);
    }
    eb.s_off = s_start;

    // --- actualizar entry_bin ---
    uint32_t end_pos = ftell(fp);
    eb.buffer_off = eb.k_off;            // inicio del buffer de datos dinámicos
    eb.buffer_size = end_pos - entry_start;

    fseek(fp, entry_start, SEEK_SET);
    fwrite(&eb, sizeof(entry_bin), 1, fp);
    fseek(fp, 0, SEEK_END);

    return entry_start;
}

uint32_t write_entry_with_index(FILE *fp_bin, FILE *fp_idx, const entry *e) {
    // Escribimos el entry en el archivo principal
    uint32_t offset = write_entry(fp_bin, e);

    // Escribimos el offset absoluto en el archivo de índice
    entry_index idx;
    idx.offset = offset;
    fwrite(&idx, sizeof(entry_index), 1, fp_idx);

    return offset;
}