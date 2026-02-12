#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../kotoba-core/include/types.h"
#include "../../kotoba-core/include/writer.h"
#include "../../kotoba-core/include/loader.h"
#include "../../kotoba-core/include/viewer.h"
#include "../../kotoba-core/include/kana.h"
#include "../../kotoba-core/include/index.h"
#include "../../kotoba-core/include/index_search.h"
#include "../../kotoba-core/include/srs.h"
#include "../../kotoba-core/include/srs_sync.h"

#include "../../cli/include/build.h"

const char *dict_path = "dict.kotoba";
const char *idx_path = "dict.kotoba.idx";

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
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

/* ---------- CLI ---------- */
int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Set Windows console to UTF-8 for input/output */
    system("chcp 65001");
#endif

    if (argc < 2)
    {
        fprintf(stderr,
                "Usage:  build-invx <tsv> out.invx\n"
                "        search <index.invx> <query>\n"
                "        srs\n"
                "        build-tsv\n");
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
        size_t n = 0;

        GramMode gram_mode;
        bool read_fourth_col = false;

        if (strcmp(mode, "jp") == 0)
        {
            /* kanji + reading unified */
            gram_mode = GRAM_JP;    /* unigrams + bigrams */
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
                read_fourth_col) != 0)
        {
            return 1;
        }

        int rc = index_build_from_pairs(
            out,
            texts,
            ids,
            meta1,
            meta2,
            n,
            gram_mode);

        free_pairs(texts, ids, meta1, meta2, n);
        return rc == 0 ? 0 : 1;
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
        init_search_context(&ctx, languages, &d, page_size);

        // warm-up
        // warm_up_minimal(&ctx);

        // measure time
        clock_t start_time = clock();
        query_results(&ctx, query);
        clock_t end_time = clock();
        double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Query executed in %.2f milliseconds\n", elapsed_time * 1000);
        query_next_page(&ctx);

        printf("Results:\n");
        for (uint32_t i = 0; i < ctx.results_processed; ++i)
        {
            print_entry(&d, ctx.results_doc_ids[i]);
        }

        printf("Results left: %u\n", ctx.results_left);

        return 0;
    }
    else if (strcmp(argv[1], "srs") == 0)
    {
        srs_profile srs;
        srs_sync sync;

        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);
        const uint32_t DICT_SIZE = d.entry_count;

        /* ───────────── sync init ───────────── */

        if (!srs_sync_open(&sync,
                           "events.dat",
                           "snapshot.dat",
                           1, /* device_id */
                           DICT_SIZE))
        {
            fprintf(stderr, "failed to init sync\n");
            return 1;
        }

        /* reconstruir estado desde snapshot + eventos */
        if (!srs_sync_rebuild(&sync, &srs, DICT_SIZE))
        {
            fprintf(stderr, "failed to rebuild srs\n");
            return 1;
        }

        char cmd[64];
        srs_prompt();

        uint64_t now = srs_now(); /* tiempo lógico */

        while (1)
        {
            printf("\n[now=%llu] ", (unsigned long long)now);
            srs_print_time(now);
            printf(" > ");

            if (scanf("%63s", cmd) != 1)
                break;

            /* ───────────── add ───────────── */
            if (strcmp(cmd, "add") == 0)
            {
                uint32_t id;
                scanf("%u", &id);

                if (id >= DICT_SIZE)
                {
                    printf("invalid id\n");
                    continue;
                }

                if (srs_sync_add(&sync, &srs, id, now))
                    printf("added entry %u\n", id);
                else
                    printf("already in SRS\n");
            }

            /* ───────────── study ───────────── */
            else if (strcmp(cmd, "study") == 0)
            {
                srs_review r;

                while (srs_peek_due(&srs, now))
                {
                    srs_pop_due_review(&srs, &r);

                    printf("\nEntry ID: %u\n", r.item->entry_id);
                    printf("State: %s\n",
                           r.item->state == SRS_LEARNING ? "LEARNING" : "REVIEW");
                    printf("Interval: %u days\n", r.item->interval);
                    printf("Ease: %.2f\n", r.item->ease);

                    printf("Quality (0=again,3=hard,4=good,5=easy): ");
                    int q;
                    scanf("%d", &q);

                    srs_quality quality;
                    switch (q)
                    {
                    case 3:
                        quality = SRS_HARD;
                        break;
                    case 4:
                        quality = SRS_GOOD;
                        break;
                    case 5:
                        quality = SRS_EASY;
                        break;
                    default:
                        quality = SRS_AGAIN;
                        break;
                    }

                    /* ⚠️ ahora pasa por sync */
                    srs_sync_review(&sync,
                                    &srs,
                                    r.item->entry_id,
                                    quality,
                                    now);

                    /* reinsertar en heap */
                    srs_requeue(&srs, r.index);
                }
            }

            /* ───────────── advance ───────────── */
            else if (strcmp(cmd, "advance") == 0)
            {
                char unit[16];
                uint32_t n;
                scanf("%u %15s", &n, unit);

                if (strcmp(unit, "min") == 0)
                    now += n * 60;
                else if (strcmp(unit, "hour") == 0)
                    now += n * 3600;
                else if (strcmp(unit, "day") == 0)
                    now += n * 86400;
                else
                    printf("unknown unit (min|hour|day)\n");
            }

            /* ───────────── due ───────────── */
            else if (strcmp(cmd, "due") == 0)
            {
                for (uint32_t i = 0; i < srs.count; ++i)
                {
                    if (srs.items[i].due <= now)
                    {
                        printf("entry %u due now\n",
                               srs.items[i].entry_id);
                    }
                }
            }

            /* ───────────── to-review ───────────── */
            else if (strcmp(cmd, "to-review") == 0)
            {
                for (uint32_t i = 0; i < srs.count; ++i)
                {
                    uint64_t due = srs.items[i].due;
                    if (due > now)
                    {
                        uint64_t seconds_left = due - now;
                        uint32_t days_left =
                            (seconds_left + 86399) / 86400;

                        printf("entry %u: %u days left\n",
                               srs.items[i].entry_id,
                               days_left);
                    }
                }
            }

            /* ───────────── rebuild (debug) ───────────── */
            else if (strcmp(cmd, "rebuild") == 0)
            {
                srs_free(&srs);
                srs_sync_rebuild(&sync, &srs, DICT_SIZE);
                printf("rebuilt from events\n");
            }

            /* ───────────── quit ───────────── */
            else if (strcmp(cmd, "quit") == 0)
                break;

            else
            {
                printf("unknown command\n");
                srs_prompt();
            }
        }

        srs_free(&srs);
        srs_sync_close(&sync);
        return 0;
    }
    else if (strcmp(argv[1], "build-tsv") == 0)
    { // generate all tsv

        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);
        TrieContext ctx;
        build_trie(&ctx);

        const char *jp_out = "tsv/jp.tsv";
        char **glosses_out;
        glosses_out = malloc(sizeof(char *) * KOTOBA_LANG_COUNT);
        const char *lang_names[] = {
            "en", "fr", "de", "ru", "es", "pt", "it", "nl", "hu", "sv",
            "cs", "pl", "ro", "he", "ar", "tr", "th", "vi", "id", "ms",
            "ko", "zh", "zh_cn", "zh_tw", "fa", "eo", "slv", "unk"};
        for (int i = 0; i < KOTOBA_LANG_COUNT - 1; ++i)
        {
            glosses_out[i] = malloc(64);
            snprintf(glosses_out[i], 64, "tsv/gloss_%s.tsv", lang_names[i]);
        }

        FILE *jp_fp = fopen(jp_out, "w");
        FILE *gloss_fps[KOTOBA_LANG_COUNT - 1];
        for (int i = 0; i < KOTOBA_LANG_COUNT - 1; ++i)
        {
            gloss_fps[i] = fopen(glosses_out[i], "w");
        }

        for (uint32_t i = 0; i < d.entry_count; ++i)
        {
            const entry_bin *e = kotoba_entry(&d, i);
            char str[64][1024];
            int str_count = 0;

            if (e->k_elements_count > 0)
            {
                const k_ele_bin *k_ele = kotoba_k_ele(&d, e, 0);
                for (int j = 0; j < e->k_elements_count; ++j)
                {
                    kotoba_str keb = kotoba_keb(&d, &k_ele[j]);
                    fprintf(jp_fp, "%u\t%.*s\t%u\t%u\n", i, (int)keb.len, keb.ptr, j, TYPE_KANJI);
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
                        fprintf(jp_fp, "%u\t%s\t%u\t%u\n", i, str[j], j, TYPE_READING);
                    }
                }
            }
            for (int s = 0; s < e->senses_count; ++s)
            {
                const sense_bin *sense = kotoba_sense(&d, e, s);
                for (int g = 0; g < sense->gloss_count; ++g)
                {
                    kotoba_str gloss = kotoba_gloss(&d, sense, g);
                    for (int lang = 0; lang < KOTOBA_LANG_COUNT - 1; ++lang)
                    {
                        if (sense->lang == lang)
                        {
                            fprintf(gloss_fps[lang], "%u\t%.*s\t%u\t%u\n", i, (int)gloss.len, gloss.ptr, s, g);
                        }
                    }
                }
            }
        }

        printf("TSV files generated successfully.\n");
        fclose(jp_fp);
        for (int i = 0; i < KOTOBA_LANG_COUNT - 1; ++i)
        {
            fclose(gloss_fps[i]);
        }

        return 0;
    }
    else if (strcmp(argv[1], "test") == 0)
    {

        // test new kana.h
        void run_test(const TrieContext *ctx, const char *input, const char *expected)
        {
            char buffer[512];

            mixed_to_hiragana(ctx, input, buffer, sizeof(buffer));

            if (strcmp(buffer, expected) == 0)
            {
                printf("✅ PASS: %-20s → %s\n", input, buffer);
            }
            else
            {
                printf("❌ FAIL: %-20s\n", input);
                printf("   got:      %s\n", buffer);
                printf("   expected: %s\n", expected);
                exit(1);
            }
        }

        TrieContext ctx;
        build_trie(&ctx);

        run_test(&ctx, "ka", "か");
        run_test(&ctx, "shi", "し");
        run_test(&ctx, "si", "し");
        run_test(&ctx, "tsu", "つ");
        run_test(&ctx, "dji", "ぢ");
        run_test(&ctx, "dzu", "づ");

        /* Youon */
        run_test(&ctx, "kyo", "きょ");
        run_test(&ctx, "ryu", "りゅ");
        run_test(&ctx, "myu", "みゅ");

        /* Vocal larga */
        run_test(&ctx, "kyuu", "きゅう");
        run_test(&ctx, "toukyou", "とうきょう");

        /* Sokuon */
        run_test(&ctx, "kitte", "きって");
        run_test(&ctx, "gakkou", "がっこう");

        /* N */
        run_test(&ctx, "kan", "かん");
        run_test(&ctx, "shin'you", "しんよう");
        run_test(&ctx, "tenno", "てんの");

        /* Katakana */
        run_test(&ctx, "カレー", "かれー");
        run_test(&ctx, "スーパー", "すーぱー");

        run_test(&ctx, "ji", "じ");
        run_test(&ctx, "dji", "ぢ");

        run_test(&ctx, "zu", "ず");
        run_test(&ctx, "かれ", "かれ");
        run_test(&ctx, "kare", "かれ");

        printf("\n🎉 All tests passed.\n");
        return 0;
    }

    /* ---------- unknown ---------- */
    else
    {
        fprintf(stderr, "unknown command\n");
        return 1;
    }
}
