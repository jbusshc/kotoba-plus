#ifndef KOTOBA_KANA_H
#define KOTOBA_KANA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "kotoba.h"

    /* ============================================================================ */
    /* UTF-8 decode */
    /* ============================================================================ */

    static const char *utf8_decode(const char *s, uint32_t *cp)
    {
        unsigned char c = (unsigned char)s[0];

        if (c < 0x80)
        {
            *cp = c;
            return s + 1;
        }
        if ((c & 0xE0) == 0xC0)
        {
            *cp = ((c & 0x1F) << 6) | (s[1] & 0x3F);
            return s + 2;
        }
        if ((c & 0xF0) == 0xE0)
        {
            *cp = ((c & 0x0F) << 12) |
                  ((s[1] & 0x3F) << 6) |
                  (s[2] & 0x3F);
            return s + 3;
        }
        if ((c & 0xF8) == 0xF0)
        {
            *cp = ((c & 0x07) << 18) |
                  ((s[1] & 0x3F) << 12) |
                  ((s[2] & 0x3F) << 6) |
                  (s[3] & 0x3F);
            return s + 4;
        }

        *cp = 0xFFFD;
        return s + 1;
    }

    /* ============================================================================ */
    /* Kana normalization */
    /* ============================================================================ */

    static uint32_t kana_to_hiragana(uint32_t cp)
    {
        /* Katakana → Hiragana */
        if (cp >= 0x30A1 && cp <= 0x30F6)
            return cp - 0x60;
        return cp;
    }

    static uint32_t normalize_kana(uint32_t cp)
    {
        return kana_to_hiragana(cp);
    }

    /* ============================================================================ */
    /* Hiragana → vowel (for prolongation mark) */
    /* ============================================================================ */

    static uint32_t hiragana_vowel(uint32_t cp)
    {
        // Rango principal de Hiragana: 0x3041 a 0x3096
        if (cp < 0x3041 || cp > 0x3096)
            return 0;

        // Tabla de mapeo: 0=ninguna, 1=a, 2=i, 3=u, 4=e, 5=o
        // Basada en el orden exacto del estándar Unicode
        static const uint8_t vowel_map[] = {
            1, 1, 2, 2, 3, 3, 4, 4, 5, 5,                // 3041 - 304A (a, i, u, e, o small/large)
            1, 1, 2, 2, 3, 3, 4, 4, 5, 5,                // 304B - 3054 (ka, ga, ki, gi...)
            1, 1, 2, 2, 3, 3, 4, 4, 5, 5,                // 3055 - 305E (sa, za, shi, ji...)
            1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5,          // 305F - 306A (ta, da, chi, ji, tsu, tsu_s, te, de, to, do, na)
            2, 3, 4, 5,                                  // 306B - 306E (ni, nu, ne, no)
            1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, // 306F - 307D (ha, ba, pa...)
            2, 3, 4, 5,                                  // 307E - 3081 (mi, mu, me, mo)
            1, 1, 3, 3, 5, 5,                            // 3082 - 3088 (ya, yu, yo small/large)
            1, 2, 3, 4, 5,                               // 3089 - 308D (ra, ri, ru, re, ro)
            1, 1, 2, 5, 0                                // 308E - 3093 (wa, wa_s, wi, we, wo, n)
        };

        uint32_t result_map[] = {0, 0x3042, 0x3044, 0x3046, 0x3048, 0x304A};

        int index = cp - 0x3041;
        if (index >= sizeof(vowel_map))
            return 0;

        return result_map[vowel_map[index]];
    }

    /* ============================================================================ */
    /* Mixed input → hiragana normalized */
    /* ============================================================================ */

    /* ============================================================
     * UTF-8 helpers
     * ============================================================ */

    static inline void emit_utf8(uint32_t cp, char **out, size_t *left)
    {
        if (cp < 0x80 && *left >= 1)
        {
            *(*out)++ = (char)cp;
            (*left)--;
        }
        else if (cp < 0x800 && *left >= 2)
        {
            *(*out)++ = 0xC0 | (cp >> 6);
            *(*out)++ = 0x80 | (cp & 0x3F);
            (*left) -= 2;
        }
        else if (*left >= 3)
        {
            *(*out)++ = 0xE0 | (cp >> 12);
            *(*out)++ = 0x80 | ((cp >> 6) & 0x3F);
            *(*out)++ = 0x80 | (cp & 0x3F);
            (*left) -= 3;
        }
    }

    /* ============================================================
     * Character helpers
     * ============================================================ */

    static inline int is_vowel(char c)
    {
        c = (char)tolower((unsigned char)c);
        return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o';
    }

    static inline int is_consonant(char c)
    {
        c = (char)tolower((unsigned char)c);
        return (c >= 'a' && c <= 'z') && !is_vowel(c);
    }

    /* ============================================================
     * Romaji table (length is explicit, no strlen)
     * ============================================================ */

#define ALPHABET 26
#define MAX_NODES 512

    typedef struct
    {
        uint16_t child[ALPHABET];
        uint16_t out1;
        uint16_t out2;
        uint8_t out_len;
    } TrieNode;

    typedef struct
    {
        TrieNode trie[MAX_NODES];
        uint16_t node_count;
    } TrieContext;

    static inline int idx(char c) { return c - 'a'; }

    static uint16_t new_node(TrieContext *ctx)
    {
        return ctx->node_count++;
    }

    static void trie_insert(TrieContext *ctx, const char *roma, uint16_t cp1, uint16_t cp2, uint8_t len)
    {
        uint16_t cur = 0;

        for (size_t i = 0; roma[i]; i++)
        {
            int k = idx(roma[i]);
            if (k < 0 || k >= 26)
                return;

            if (!ctx->trie[cur].child[k])
                ctx->trie[cur].child[k] = new_node(ctx);

            cur = ctx->trie[cur].child[k];
        }

        ctx->trie[cur].out1 = cp1;
        ctx->trie[cur].out2 = cp2;
        ctx->trie[cur].out_len = len;
    }

    /* ============================= */
    /* Construcción */
    /* ============================= */

    static void build_trie(TrieContext *ctx)
    {
        memset(ctx->trie, 0, sizeof(ctx->trie));
        ctx->node_count = 1;

        /* ===== Vocales ===== */
        trie_insert(ctx, "a", 0x30A2, 0, 1);
        trie_insert(ctx, "i", 0x30A4, 0, 1);
        trie_insert(ctx, "u", 0x30A6, 0, 1);
        trie_insert(ctx, "e", 0x30A8, 0, 1);
        trie_insert(ctx, "o", 0x30AA, 0, 1);

        /* ===== K ===== */
        trie_insert(ctx, "ka", 0x30AB, 0, 1);
        trie_insert(ctx, "ki", 0x30AD, 0, 1);
        trie_insert(ctx, "ku", 0x30AF, 0, 1);
        trie_insert(ctx, "ke", 0x30B1, 0, 1);
        trie_insert(ctx, "ko", 0x30B3, 0, 1);

        trie_insert(ctx, "kya", 0x30AD, 0x30E3, 2);
        trie_insert(ctx, "kyu", 0x30AD, 0x30E5, 2);
        trie_insert(ctx, "kyo", 0x30AD, 0x30E7, 2);

        /* kwa series */
        trie_insert(ctx, "kwa", 0x30AF, 0x30A1, 2);
        trie_insert(ctx, "kwi", 0x30AF, 0x30A3, 2);
        trie_insert(ctx, "kwe", 0x30AF, 0x30A7, 2);
        trie_insert(ctx, "kwo", 0x30AF, 0x30A9, 2);

        /* ===== G ===== */
        trie_insert(ctx, "ga", 0x30AC, 0, 1);
        trie_insert(ctx, "gi", 0x30AE, 0, 1);
        trie_insert(ctx, "gu", 0x30B0, 0, 1);
        trie_insert(ctx, "ge", 0x30B2, 0, 1);
        trie_insert(ctx, "go", 0x30B4, 0, 1);

        trie_insert(ctx, "gya", 0x30AE, 0x30E3, 2);
        trie_insert(ctx, "gyu", 0x30AE, 0x30E5, 2);
        trie_insert(ctx, "gyo", 0x30AE, 0x30E7, 2);

        trie_insert(ctx, "gwa", 0x30B0, 0x30A1, 2);
        trie_insert(ctx, "gwi", 0x30B0, 0x30A3, 2);
        trie_insert(ctx, "gwe", 0x30B0, 0x30A7, 2);
        trie_insert(ctx, "gwo", 0x30B0, 0x30A9, 2);

        /* ===== S ===== */
        trie_insert(ctx, "sa", 0x30B5, 0, 1);
        trie_insert(ctx, "shi", 0x30B7, 0, 1);
        trie_insert(ctx, "si", 0x30B7, 0, 1);
        trie_insert(ctx, "su", 0x30B9, 0, 1);
        trie_insert(ctx, "se", 0x30BB, 0, 1);
        trie_insert(ctx, "so", 0x30BD, 0, 1);

        trie_insert(ctx, "sha", 0x30B7, 0x30E3, 2);
        trie_insert(ctx, "shu", 0x30B7, 0x30E5, 2);
        trie_insert(ctx, "sho", 0x30B7, 0x30E7, 2);

        trie_insert(ctx, "she", 0x30B7, 0x30A7, 2);

        /* ===== T ===== */
        trie_insert(ctx, "ta", 0x30BF, 0, 1);
        trie_insert(ctx, "chi", 0x30C1, 0, 1);
        trie_insert(ctx, "ti", 0x30C6, 0x30A3, 2);
        trie_insert(ctx, "tsu", 0x30C4, 0, 1);
        trie_insert(ctx, "tu", 0x30C8, 0x30A5, 2);
        trie_insert(ctx, "te", 0x30C6, 0, 1);
        trie_insert(ctx, "to", 0x30C8, 0, 1);

        trie_insert(ctx, "cha", 0x30C1, 0x30E3, 2);
        trie_insert(ctx, "chu", 0x30C1, 0x30E5, 2);
        trie_insert(ctx, "cho", 0x30C1, 0x30E7, 2);
        trie_insert(ctx, "che", 0x30C1, 0x30A7, 2);

        trie_insert(ctx, "tsa", 0x30C4, 0x30A1, 2);
        trie_insert(ctx, "tsi", 0x30C4, 0x30A3, 2);
        trie_insert(ctx, "tse", 0x30C4, 0x30A7, 2);
        trie_insert(ctx, "tso", 0x30C4, 0x30A9, 2);

        /* ===== D ===== */
        trie_insert(ctx, "da", 0x30C0, 0, 1);
        trie_insert(ctx, "de", 0x30C7, 0, 1);
        trie_insert(ctx, "do", 0x30C9, 0, 1);

        trie_insert(ctx, "di", 0x30C7, 0x30A3, 2);
        trie_insert(ctx, "du", 0x30C9, 0x30A5, 2);

        trie_insert(ctx, "dji", 0x30C2, 0, 1);

        trie_insert(ctx, "dya", 0x30C7, 0x30E3, 2);
        trie_insert(ctx, "dyu", 0x30C7, 0x30E5, 2);
        trie_insert(ctx, "dyo", 0x30C7, 0x30E7, 2);

        /* ===== F ===== */
        trie_insert(ctx, "fa", 0x30D5, 0x30A1, 2);
        trie_insert(ctx, "fi", 0x30D5, 0x30A3, 2);
        trie_insert(ctx, "fe", 0x30D5, 0x30A7, 2);
        trie_insert(ctx, "fo", 0x30D5, 0x30A9, 2);

        trie_insert(ctx, "fya", 0x30D5, 0x30E3, 2);
        trie_insert(ctx, "fyu", 0x30D5, 0x30E5, 2);
        trie_insert(ctx, "fyo", 0x30D5, 0x30E7, 2);

        /* ===== V ===== */
        trie_insert(ctx, "vu", 0x30F4, 0, 1);
        trie_insert(ctx, "va", 0x30F4, 0x30A1, 2);
        trie_insert(ctx, "vi", 0x30F4, 0x30A3, 2);
        trie_insert(ctx, "ve", 0x30F4, 0x30A7, 2);
        trie_insert(ctx, "vo", 0x30F4, 0x30A9, 2);

        trie_insert(ctx, "vya", 0x30F4, 0x30E3, 2);
        trie_insert(ctx, "vyu", 0x30F4, 0x30E5, 2);
        trie_insert(ctx, "vyo", 0x30F4, 0x30E7, 2);

        /* ===== J ===== */
        trie_insert(ctx, "ja", 0x30B8, 0x30E3, 2);
        trie_insert(ctx, "ji", 0x30B8, 0, 1);
        trie_insert(ctx, "ju", 0x30B8, 0x30E5, 2);
        trie_insert(ctx, "jo", 0x30B8, 0x30E7, 2);
        trie_insert(ctx, "je", 0x30B8, 0x30A7, 2);

        /* ===== TH / DH ===== */
        trie_insert(ctx, "tha", 0x30C6, 0x30A1, 2);
        trie_insert(ctx, "thi", 0x30C6, 0x30A3, 2);
        trie_insert(ctx, "thu", 0x30C6, 0x30A5, 2);
        trie_insert(ctx, "the", 0x30C6, 0x30A7, 2);
        trie_insert(ctx, "tho", 0x30C6, 0x30A9, 2);

        trie_insert(ctx, "dha", 0x30C7, 0x30A1, 2);
        trie_insert(ctx, "dhi", 0x30C7, 0x30A3, 2);
        trie_insert(ctx, "dhu", 0x30C7, 0x30A5, 2);
        trie_insert(ctx, "dhe", 0x30C7, 0x30A7, 2);
        trie_insert(ctx, "dho", 0x30C7, 0x30A9, 2);

        /* ===== H ===== */
        trie_insert(ctx, "ha", 0x30CF, 0, 1);
        trie_insert(ctx, "hi", 0x30D2, 0, 1);
        trie_insert(ctx, "hu", 0x30D5, 0, 1);
        trie_insert(ctx, "he", 0x30D8, 0, 1);
        trie_insert(ctx, "ho", 0x30DB, 0, 1);

        trie_insert(ctx, "hya", 0x30D2, 0x30E3, 2);
        trie_insert(ctx, "hyu", 0x30D2, 0x30E5, 2);
        trie_insert(ctx, "hyo", 0x30D2, 0x30E7, 2);

        /* ===== B ===== */
        trie_insert(ctx, "ba", 0x30D0, 0, 1);
        trie_insert(ctx, "bi", 0x30D3, 0, 1);
        trie_insert(ctx, "bu", 0x30D6, 0, 1);
        trie_insert(ctx, "be", 0x30D9, 0, 1);
        trie_insert(ctx, "bo", 0x30DC, 0, 1);

        trie_insert(ctx, "bya", 0x30D3, 0x30E3, 2);
        trie_insert(ctx, "byu", 0x30D3, 0x30E5, 2);
        trie_insert(ctx, "byo", 0x30D3, 0x30E7, 2);

        /* ===== P ===== */
        trie_insert(ctx, "pa", 0x30D1, 0, 1);
        trie_insert(ctx, "pi", 0x30D4, 0, 1);
        trie_insert(ctx, "pu", 0x30D7, 0, 1);
        trie_insert(ctx, "pe", 0x30DA, 0, 1);
        trie_insert(ctx, "po", 0x30DD, 0, 1);

        trie_insert(ctx, "pya", 0x30D4, 0x30E3, 2);
        trie_insert(ctx, "pyu", 0x30D4, 0x30E5, 2);
        trie_insert(ctx, "pyo", 0x30D4, 0x30E7, 2);

        /* ===== M ===== */
        trie_insert(ctx, "ma", 0x30DE, 0, 1);
        trie_insert(ctx, "mi", 0x30DF, 0, 1);
        trie_insert(ctx, "mu", 0x30E0, 0, 1);
        trie_insert(ctx, "me", 0x30E1, 0, 1);
        trie_insert(ctx, "mo", 0x30E2, 0, 1);

        trie_insert(ctx, "mya", 0x30DF, 0x30E3, 2);
        trie_insert(ctx, "myu", 0x30DF, 0x30E5, 2);
        trie_insert(ctx, "myo", 0x30DF, 0x30E7, 2);

        /* ===== Y ===== */
        trie_insert(ctx, "ya", 0x30E4, 0, 1);
        trie_insert(ctx, "yu", 0x30E6, 0, 1);
        trie_insert(ctx, "yo", 0x30E8, 0, 1);

        /* ===== R ===== */
        trie_insert(ctx, "ra", 0x30E9, 0, 1);
        trie_insert(ctx, "ri", 0x30EA, 0, 1);
        trie_insert(ctx, "ru", 0x30EB, 0, 1);
        trie_insert(ctx, "re", 0x30EC, 0, 1);
        trie_insert(ctx, "ro", 0x30ED, 0, 1);

        trie_insert(ctx, "rya", 0x30EA, 0x30E3, 2);
        trie_insert(ctx, "ryu", 0x30EA, 0x30E5, 2);
        trie_insert(ctx, "ryo", 0x30EA, 0x30E7, 2);

        /* ===== W ===== */
        trie_insert(ctx, "wa", 0x30EF, 0, 1);
        trie_insert(ctx, "wi", 0x30F3, 0x30A3, 2);
        trie_insert(ctx, "we", 0x30F3, 0x30A7, 2);
        trie_insert(ctx, "wo", 0x30F2, 0, 1);

        /* ===== N ===== */
        trie_insert(ctx, "n", 0x30F3, 0, 1);

        trie_insert(ctx, "na", 0x30CA, 0, 1);
        trie_insert(ctx, "ni", 0x30CB, 0, 1);
        trie_insert(ctx, "nu", 0x30CC, 0, 1);
        trie_insert(ctx, "ne", 0x30CD, 0, 1);
        trie_insert(ctx, "no", 0x30CE, 0, 1);

        trie_insert(ctx, "nya", 0x30CB, 0x30E3, 2);
        trie_insert(ctx, "nyu", 0x30CB, 0x30E5, 2);
        trie_insert(ctx, "nyo", 0x30CB, 0x30E7, 2);

        /* ===== Z ===== */
        trie_insert(ctx, "za", 0x30B6, 0, 1);
        trie_insert(ctx, "zi", 0x30B8, 0, 1);
        trie_insert(ctx, "zu", 0x30BA, 0, 1);
        trie_insert(ctx, "ze", 0x30BC, 0, 1);
        trie_insert(ctx, "zo", 0x30BE, 0, 1);

        trie_insert(ctx, "zya", 0x30B8, 0x30E3, 2);
        trie_insert(ctx, "zyu", 0x30B8, 0x30E5, 2);
        trie_insert(ctx, "zyo", 0x30B8, 0x30E7, 2);

        /* ===== Special ===== */
        trie_insert(ctx, "-", 0x30FC, 0, 1);
        trie_insert(ctx, "dzu", 0x30C5, 0, 1);
    }


    /* ============================================================
     * Main conversion
     * ============================================================ */

    KOTOBA_API void mixed_to_hiragana(const TrieContext *ctx, const char *input, char *output, size_t out_size);

    KOTOBA_API void vowel_prolongation_mark(const char *input, char *output, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* KOTOBA_KANA_H */
