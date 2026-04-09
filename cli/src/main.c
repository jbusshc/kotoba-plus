#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../core/include/types.h"
#include "../../core/include/writer.h"
#include "../../core/include/loader.h"
#include "../../core/include/viewer.h"
#include "../../core/include/kana.h"
#include "../../core/include/index.h"
#include "../../core/include/index_search.h"

#include "../include/build.h"

#include "../include/parser.h"

const char *dict_path = "dict.kotoba";
const char *idx_path = "dict.kotoba.idx";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#define _POSIX_C_SOURCE 200809L

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#include <time.h>

void srs_prompt(void)
{
    printf("\nCommands:\n");
    printf("  add <id>        add entry to SRS\n");
    printf("  study           study due cards\n");
    printf("  day <n>         advance n days\n");
    // printf("  save            save profile\n");
    // printf("  load            load profile\n");
    printf("  to-review      list entries in SRS with days left\n");
    printf("  due            list entries due today or overdue\n");
    printf("  quit\n");
}

void deep(int level, char* base) { // for estimating stack usage
    char marker;
    if (level == 0) {
        printf("Stack usado aprox: %ld bytes\n", base - &marker);
        return;
    }
    deep(level - 1, base);
}

/* ---------- CLI ---------- */
int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Set Windows console to UTF-8 for input/output */
    system("chcp 65001");
#endif
    char base;

    if (argc < 2)
    {
        fprintf(stderr,
                "Usage:  build-invx <tsv> out.invx\n"
                "        search <index.invx> <query>\n"
                "        build-tsv\n"
                "        build-dict\n"
                "        patch\n"
            );
        return 1;
    }

    /* ---------- build ---------- */
    if (strcmp(argv[1], "build-invx") == 0)
    {
        if (argc != 5)
        {
            fprintf(stderr,
                    "build-invx <tsv> jp out.invx\n"
                    "build-invx <tsv> gloss out.invx\n");
            return 1;
        }

        const char *tsv = argv[2];
        const char *mode = argv[3];
        const char *out = argv[4];

        const char **texts = NULL;
        uint32_t *ids = NULL;
        uint8_t *meta1 = NULL;
        uint8_t *meta2 = NULL;
        uint8_t *common = NULL;
        size_t n = 0;

        GramMode gram_mode;
        bool read_fourth_col = false;

        if (strcmp(mode, "jp") == 0)
        {
            /* kanji + reading unified */
            gram_mode = GRAM_JP_ALL;    /* unigrams + bigrams */
            read_fourth_col = true; /* meta2 = TYPE_KANJI / TYPE_READING */
        }
        else if (strcmp(mode, "gloss") == 0)
        {
            gram_mode = GRAM_GLOSS_AUTO;
            read_fourth_col = true; /* meta1 = sense_id, meta2 = gloss_id */
        }
        else
        {
            fprintf(stderr, "invalid mode: %s\n", mode);
            return 1;
        }

        if (read_tsv_pairs(
                tsv,
                &texts,
                &ids,
                &meta1,
                &meta2,
                &n,
                read_fourth_col,
                &common) != 0)
        {
            return 1;
        }

        int rc = index_build_from_pairs(
            out,
            texts,
            ids,
            meta1,
            meta2,
            common,
            n,
            gram_mode);

        free_pairs(texts, ids, meta1, meta2, common, n);
        return rc == 0 ? 0 : 1;
    } else if (strcmp(argv[1], "build-dict") == 0)
    {

        build_dict();
        return 0;
    }

    /* ---------- search ---------- */
    else if (strcmp(argv[1], "search") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "search <query> \n");
            return 1;
        }

        const char *query_file = argv[2];

        char *query = read_utf8_file(query_file);
        if (!query)
        {
            fprintf(stderr, "failed read query file\n");
            return 1;
        }

        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);

        bool languages[KOTOBA_LANG_COUNT] = {0};
        languages[KOTOBA_LANG_EN] = true;
        languages[KOTOBA_LANG_ES] = true;

        int page_size = 10;
        struct SearchContext ctx;
        init_search_context(&ctx, languages, &d, page_size, 0);



        // measure time
        clock_t start_time = clock();
        query_results(&ctx, query);
        clock_t end_time = clock();
        double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Query executed in %.2f milliseconds\n", elapsed_time * 1000);
        printf("results processed: %u\n", ctx.page_result_count);
        printf("results left: %u\n", ctx.results_left);
        query_next_page(&ctx);


        printf("Results left: %u\n", ctx.results_left);
        deep(1, &base); // estimar uso de stack para index_build_from_pairs

        return 0;
    } else if (strcmp(argv[1], "build-tsv") == 0)
    {
        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);
        TrieContext ctx;
        build_trie(&ctx);

        char* lang_fnames[KOTOBA_LANG_COUNT] = {
            "gloss_en.tsv",     // KOTOBA_LANG_EN
            "gloss_fr.tsv",     // KOTOBA_LANG_FR
            "gloss_de.tsv",     // KOTOBA_LANG_DE
            "gloss_ru.tsv",     // KOTOBA_LANG_RU
            "gloss_es.tsv",     // KOTOBA_LANG_ES
            "gloss_pt.tsv",     // KOTOBA_LANG_PT
            "gloss_it.tsv",     // KOTOBA_LANG_IT
            "gloss_nl.tsv",     // KOTOBA_LANG_NL
            "gloss_hu.tsv",     // KOTOBA_LANG_HU
            "gloss_sv.tsv",     // KOTOBA_LANG_SV
            "gloss_cs.tsv",     // KOTOBA_LANG_CS
            "gloss_pl.tsv",     // KOTOBA_LANG_PL
            "gloss_ro.tsv",     // KOTOBA_LANG_RO
            "gloss_he.tsv",     // KOTOBA_LANG_HE
            "gloss_ar.tsv",     // KOTOBA_LANG_AR
            "gloss_tr.tsv",     // KOTOBA_LANG_TR
            "gloss_th.tsv",     // KOTOBA_LANG_TH
            "gloss_vi.tsv",     // KOTOBA_LANG_VI
            "gloss_id.tsv",     // KOTOBA_LANG_ID
            "gloss_ms.tsv",     // KOTOBA_LANG_MS
            "gloss_ko.tsv",     // KOTOBA_LANG_KO
            "gloss_zh.tsv",     // KOTOBA_LANG_ZH
            "gloss_zh_cn.tsv",  // KOTOBA_LANG_ZH_CN
            "gloss_zh_tw.tsv",  // KOTOBA_LANG_ZH_TW
            "gloss_fa.tsv",     // KOTOBA_LANG_FA
            "gloss_eo.tsv",     // KOTOBA_LANG_EO
            "gloss_slv.tsv",    // KOTOBA_LANG_SLV
            "gloss_unk.tsv"     // KOTOBA_LANG_UNK
        };

        char* jp_out = "jp.tsv";

        FILE *jp_fp = fopen(jp_out, "w");
        FILE *gloss_fps[KOTOBA_LANG_COUNT];
        for (int i = 0; i < KOTOBA_LANG_COUNT; ++i)
        {
            gloss_fps[i] = fopen(lang_fnames[i], "w");
        }
    
        printf("Total entries: %u\n", d.bin_header->entry_count);
        for (uint32_t i = 0; i < d.bin_header->entry_count; ++i)
        {
            char str[64][1024];
            int str_count = 0;
            if (i % 1000 == 0)
                printf("Processing entry %u/%u\n", i, d.bin_header->entry_count);
            
            const entry_bin *e = kotoba_entry_by_index(&d, i);

            if (e->k_elements_count > 0)
            {
                const k_ele_bin *k_ele = kotoba_k_ele(&d, e, 0);
                for (int j = 0; j < e->k_elements_count; ++j)
                {
                    kotoba_str keb = kotoba_keb(&d, &k_ele[j]);
                    fprintf(jp_fp, "%u\t%.*s\t%u\t%u\t%u\t%u\n", e->ent_seq, (int)keb.len, keb.ptr, j, TYPE_KANJI, keb.len, e->priority);
                }
            }

            if (e->r_elements_count > 0)
            {
                const r_ele_bin *r_ele = kotoba_r_ele(&d, e, 0);
                for (int j = 0; j < e->r_elements_count; ++j)
                {
                    kotoba_str reb = kotoba_reb(&d, &r_ele[j]);
                    char reb_str[256];
                    strncpy(reb_str, reb.ptr, reb.len);
                    reb_str[reb.len] = '\0';
                    char normalized[256];
                    mixed_to_hiragana(&ctx, reb_str, normalized, sizeof(normalized));

                    memcpy(str[str_count++], normalized, sizeof(normalized));
                }

                for (int j = 0; j < str_count; ++j)
                {
                    bool is_duplicate = false;
                    for (int k = 0; k < j; ++k)
                    {
                        if (strcmp(str[k], str[j]) == 0)
                        {
                            is_duplicate = true;
                            break;
                        }
                    }
                    if (!is_duplicate)
                    {
                        fprintf(jp_fp, "%u\t%s\t%u\t%u\t%u\t%u\n", e->ent_seq, str[j], j, TYPE_READING, strlen(str[j]), e->priority);
                    }
                }
            }

            for (int s = 0; s < e->senses_count; ++s)
            {
                const sense_bin *sense = kotoba_sense(&d, e, s);
                for (int g = 0; g < sense->gloss_count; ++g)
                {
                    kotoba_str gloss = kotoba_gloss(&d, sense, g);
                    int lang = sense->lang < KOTOBA_LANG_COUNT ? sense->lang : KOTOBA_LANG_UNK;
                    fprintf( gloss_fps[lang], "%u\t%.*s\t%u\t%u\t%u\t%u\n", e->ent_seq, (int)gloss.len, gloss.ptr, s, g, gloss.len, e->priority);
                }
            }
        }

        printf("Iteration complete\n");
        kotoba_dict_close(&d);
        

    }
    else if (strcmp(argv[1], "patch") == 0)
    {
        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);

        kotoba_writer w;
        kotoba_writer_open_patch(&w, dict_path, idx_path, &d);

        kotoba_writer_patch_entries(&w, &d); // 10 min, N² complexity, should be optimized 

        print_entry(&d, 5);
        kotoba_dict_close(&d);
        return 0;
    } else if (strcmp(argv[1], "test") == 0)
    {
        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);

        const entry_bin *e = kotoba_dict_get_entry(&d, 0);
        
        printf("Entry 0: ent_seq = %u, k_elements_count = %u, r_elements_count = %u, senses_count = %u\n",
               e->ent_seq, e->k_elements_count, e->r_elements_count, e->senses_count);
        print_entry(&d, 0);

        kotoba_dict_close(&d);
        return 0;
    }

    /* ---------- unknown ---------- */
    else
    {
        fprintf(stderr, "unknown command\n");
        return 1;
    }
}
