#ifndef KOTOBA_KANA_H
#define KOTOBA_KANA_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>


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
    switch (cp)
    {
    /* a */
    case 0x3042:
    case 0x304B:
    case 0x3055:
    case 0x305F:
    case 0x306A:
    case 0x306F:
    case 0x307E:
    case 0x3084:
    case 0x3089:
    case 0x308F:
    case 0x3041:
    case 0x3083:
        return 0x3042;

    /* i */
    case 0x3044:
    case 0x304D:
    case 0x3057:
    case 0x3061:
    case 0x306B:
    case 0x3072:
    case 0x307F:
    case 0x308A:
    case 0x3043:
        return 0x3044;

    /* u */
    case 0x3046:
    case 0x304F:
    case 0x3059:
    case 0x3064:
    case 0x306C:
    case 0x3075:
    case 0x3080:
    case 0x3086:
    case 0x308B:
    case 0x3045:
    case 0x3085:
        return 0x3046;

    /* e */
    case 0x3048:
    case 0x3051:
    case 0x305B:
    case 0x3066:
    case 0x306D:
    case 0x3078:
    case 0x3081:
    case 0x308C:
    case 0x3047:
        return 0x3048;

    /* o */
    case 0x304A:
    case 0x3053:
    case 0x305D:
    case 0x3068:
    case 0x306E:
    case 0x307B:
    case 0x3082:
    case 0x3088:
    case 0x308D:
    case 0x3049:
    case 0x3087:
        return 0x304A;
    }
    return 0;
}

/* ============================================================================ */
/* Mixed input → hiragana normalized */
/* ============================================================================ */

#include <stdint.h>
#include <string.h>
#include <ctype.h>

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

typedef struct {
    const char *roma;
    uint8_t len;
    uint32_t hira[2];
    uint8_t hira_len;
} RomaMap;

/* ORDER MATTERS: longest first */
static const RomaMap romaji_table[] = {
    /* yōon */
    {"kya",3,{0x304D,0x3083},2}, {"kyu",3,{0x304D,0x3085},2}, {"kyo",3,{0x304D,0x3087},2},
    {"sha",3,{0x3057,0x3083},2}, {"shu",3,{0x3057,0x3085},2}, {"sho",3,{0x3057,0x3087},2},
    {"cha",3,{0x3061,0x3083},2}, {"chu",3,{0x3061,0x3085},2}, {"cho",3,{0x3061,0x3087},2},
    {"nya",3,{0x306B,0x3083},2}, {"nyu",3,{0x306B,0x3085},2}, {"nyo",3,{0x306B,0x3087},2},
    {"hya",3,{0x3072,0x3083},2}, {"hyu",3,{0x3072,0x3085},2}, {"hyo",3,{0x3072,0x3087},2},
    {"mya",3,{0x307F,0x3083},2}, {"myu",3,{0x307F,0x3085},2}, {"myo",3,{0x307F,0x3087},2},
    {"rya",3,{0x308A,0x3083},2}, {"ryu",3,{0x308A,0x3085},2}, {"ryo",3,{0x308A,0x3087},2},
    {"gya",3,{0x304E,0x3083},2}, {"gyu",3,{0x304E,0x3085},2}, {"gyo",3,{0x304E,0x3087},2},
    {"bya",3,{0x3073,0x3083},2}, {"byu",3,{0x3073,0x3085},2}, {"byo",3,{0x3073,0x3087},2},
    {"pya",3,{0x3074,0x3083},2}, {"pyu",3,{0x3074,0x3085},2}, {"pyo",3,{0x3074,0x3087},2},

    /* specials */
    {"shi",3,{0x3057},1},
    {"chi",3,{0x3061},1},
    {"tsu",3,{0x3064},1},

    /* k */
    {"ka",2,{0x304B},1},{"ki",2,{0x304D},1},{"ku",2,{0x304F},1},{"ke",2,{0x3051},1},{"ko",2,{0x3053},1},
    /* s */
    {"sa",2,{0x3055},1},{"su",2,{0x3059},1},{"se",2,{0x305B},1},{"so",2,{0x305D},1},
    /* t */
    {"ta",2,{0x305F},1},{"te",2,{0x3066},1},{"to",2,{0x3068},1},
    /* n */
    {"na",2,{0x306A},1},{"ni",2,{0x306B},1},{"nu",2,{0x306C},1},{"ne",2,{0x306D},1},{"no",2,{0x306E},1},
    /* h */
    {"ha",2,{0x306F},1},{"hi",2,{0x3072},1},{"fu",2,{0x3075},1},{"he",2,{0x3078},1},{"ho",2,{0x307B},1},
    /* m */
    {"ma",2,{0x307E},1},{"mi",2,{0x307F},1},{"mu",2,{0x3080},1},{"me",2,{0x3081},1},{"mo",2,{0x3082},1},
    /* y */
    {"ya",2,{0x3084},1},{"yu",2,{0x3086},1},{"yo",2,{0x3088},1},
    /* r */
    {"ra",2,{0x3089},1},{"ri",2,{0x308A},1},{"ru",2,{0x308B},1},{"re",2,{0x308C},1},{"ro",2,{0x308D},1},
    /* w */
    {"wa",2,{0x308F},1},{"wo",2,{0x3092},1},
    /* g */
    {"ga",2,{0x304C},1},{"gi",2,{0x304E},1},{"gu",2,{0x3050},1},{"ge",2,{0x3052},1},{"go",2,{0x3054},1},
    /* z */
    {"za",2,{0x3056},1},{"ji",2,{0x3058},1},{"zu",2,{0x305A},1},{"ze",2,{0x305C},1},{"zo",2,{0x305E},1},
    /* d */
    {"da",2,{0x3060},1},{"de",2,{0x3067},1},{"do",2,{0x3069},1},
    /* b */
    {"ba",2,{0x3070},1},{"bi",2,{0x3073},1},{"bu",2,{0x3076},1},{"be",2,{0x3079},1},{"bo",2,{0x307C},1},
    /* p */
    {"pa",2,{0x3071},1},{"pi",2,{0x3074},1},{"pu",2,{0x3077},1},{"pe",2,{0x307A},1},{"po",2,{0x307D},1},
    /* n */
    {"n",1,{0x3093},1},
    /* vowels */
    {"a",1,{0x3042},1},{"i",1,{0x3044},1},{"u",1,{0x3046},1},{"e",1,{0x3048},1},{"o",1,{0x304A},1},
};

/* ============================================================
 * Romaji matcher
 * ============================================================ */

static int match_romaji(const char *s, uint32_t *out, int *out_len)
{
    for (size_t i = 0; i < sizeof(romaji_table)/sizeof(romaji_table[0]); i++)
    {
        const RomaMap *r = &romaji_table[i];
        if (strncasecmp(s, r->roma, r->len) == 0)
        {
            for (int j = 0; j < r->hira_len; j++)
                out[j] = r->hira[j];
            *out_len = r->hira_len;
            return r->len;
        }
    }
    return 0;
}

/* ============================================================
 * Main conversion
 * ============================================================ */

void mixed_to_hiragana(const char *input, char *output, size_t out_size)
{
    const char *s = input;
    char *out = output;
    size_t left = out_size - 1;

    while (*s && left > 0)
    {
        /* ASCII romaji path */
        if (isalpha((unsigned char)s[0]))
        {
            /* sokuon */
            if (is_consonant(s[0]) && s[0] == s[1] && s[0] != 'n')
            {
                emit_utf8(0x3063, &out, &left); /* っ */
                s++;
                continue;
            }

            /* n → ん */
            if (tolower((unsigned char)s[0]) == 'n')
            {
                if (s[1] == '\'' ||
                    (!is_vowel(s[1]) && tolower((unsigned char)s[1]) != 'y'))
                {
                    emit_utf8(0x3093, &out, &left); /* ん */
                    s += (s[1] == '\'') ? 2 : 1;
                    continue;
                }
            }

            /* normal romaji */
            uint32_t buf[2];
            int hira_len = 0;
            int consumed = match_romaji(s, buf, &hira_len);
            if (consumed > 0)
            {
                for (int i = 0; i < hira_len; i++)
                    emit_utf8(buf[i], &out, &left);
                s += consumed;
                continue;
            }
        }

        /* fallback: copy byte */
        *out++ = *s++;
        left--;
    }

    *out = '\0';
}




#endif /* KOTOBA_KANA_H */
