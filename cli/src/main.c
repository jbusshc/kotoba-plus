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
    printf("  save            save profile\n");
    printf("  load            load profile\n");
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
                "        srs\n");
        return 1;
    }

    /* ---------- build ---------- */
    if (strcmp(argv[1], "build-invx") == 0)
    {
        if (argc != 5)
        {
            fprintf(stderr,
                    "build <tsv> jp out.idx\n"
                    "build <tsv> gloss out.idx\n");
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
        uint64_t now = srs_now(); /* tiempo lógico inicial */

        kotoba_dict d;
        kotoba_dict_open(&d, dict_path, idx_path);
        const uint32_t DICT_SIZE = d.entry_count;

        if (!srs_init(&srs, DICT_SIZE))
        {
            fprintf(stderr, "failed to init srs\n");
            return 1;
        }

        char cmd[64];
        srs_prompt();

        while (1)
        {
            printf("\n[now=%llu] > ", (unsigned long long)now);

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

                if (srs_add(&srs, id, now))
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
                    srs_pop_due_review(&srs,now, &r);

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
                    case 3: quality = SRS_HARD; break;
                    case 4: quality = SRS_GOOD; break;
                    case 5: quality = SRS_EASY; break;
                    default: quality = SRS_AGAIN; break;
                    }

                    srs_answer(r.item, quality, now);

                    /* ← REINSERCIÓN CORRECTA */
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

            /* ───────────── save/load ───────────── */
            else if (strcmp(cmd, "save") == 0)
            {
                srs_save(&srs, "profile.dat");
                printf("saved\n");
            }
            else if (strcmp(cmd, "load") == 0)
            {
                srs_free(&srs);
                srs_load(&srs, "profile.dat", DICT_SIZE);
                printf("loaded\n");
            }

            else if (strcmp(cmd, "quit") == 0)
                break;

            else
            {
                printf("unknown command\n");
                srs_prompt();
            }
        }

        srs_free(&srs);
        return 0;
    }

    else if (strcmp(argv[1], "test") == 0)
    {

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

        // warm-up test query
        clock_t warm_start = clock();
        warm_up(&ctx);
        clock_t warm_end = clock();
        double warm_elapsed = (double)(warm_end - warm_start) / CLOCKS_PER_SEC;
        printf("Warm-up executed in %.2f milliseconds\n", warm_elapsed * 1000);
        // measure time
        clock_t start_time = clock();
        query_results(&ctx, query);
        clock_t end_time = clock();
        double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Query executed in %.2f milliseconds\n", elapsed_time * 1000);

        clock_t page_start = clock();
        query_next_page(&ctx);
        clock_t page_end = clock();
        double page_elapsed = (double)(page_end - page_start) / CLOCKS_PER_SEC;
        printf("Page retrieval executed in %.2f milliseconds\n", page_elapsed * 1000);

        printf("Results: %u\n", ctx.results_left);

        // test romaji to kana
        const char *romaji = "kon'nichiwa, sekai! watashi wa AI desu.";
        char kana[256] = {0};

        mixed_to_hiragana(romaji, kana, sizeof(kana));
        printf("Romaji: %s\nKana: %s\n", romaji, kana);

        return 0;
    }

    /* ---------- unknown ---------- */
    else
    {
        fprintf(stderr, "unknown command\n");
        return 1;
    }
}
