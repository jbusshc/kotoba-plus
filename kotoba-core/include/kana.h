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

static void mixed_to_hiragana(const char *input, char *output, size_t out_size)
{
    const char *s = input;
    char *out = output;
    size_t left = out_size - 1;

    uint32_t cp;
    uint32_t last_hira = 0;

    while (*s && left > 0)
    {
        const char *next = utf8_decode(s, &cp);

        /* Kanji blocks */
        int is_kanji =
            (cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0x20000 && cp <= 0x2CEAF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0x2F800 && cp <= 0x2FA1F);

        if (is_kanji)
        {
            size_t len = next - s;
            if (len > left)
                len = left;
            memcpy(out, s, len);
            out += len;
            left -= len;
            last_hira = 0;
        }
        else if (cp == 0x30FC) /* prolongation mark */
        {
            uint32_t v = hiragana_vowel(last_hira);
            if (v && left >= 3)
            {
                *out++ = 0xE0 | (v >> 12);
                *out++ = 0x80 | ((v >> 6) & 0x3F);
                *out++ = 0x80 | (v & 0x3F);
                left -= 3;
                last_hira = v;
            }
        }
        else
        {
            uint32_t hira = normalize_kana(cp);

            /* ASCII romaji (very minimal) */
            if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z'))
            {
                switch (tolower((int)cp))
                {
                case 'a':
                    hira = 0x3042;
                    break;
                case 'i':
                    hira = 0x3044;
                    break;
                case 'u':
                    hira = 0x3046;
                    break;
                case 'e':
                    hira = 0x3048;
                    break;
                case 'o':
                    hira = 0x304A;
                    break;
                default:
                    goto skip;
                }
            }

            if (hira < 0x80 && left >= 1)
            {
                *out++ = (char)hira;
                left--;
            }
            else if (hira < 0x800 && left >= 2)
            {
                *out++ = 0xC0 | (hira >> 6);
                *out++ = 0x80 | (hira & 0x3F);
                left -= 2;
            }
            else if (left >= 3)
            {
                *out++ = 0xE0 | (hira >> 12);
                *out++ = 0x80 | ((hira >> 6) & 0x3F);
                *out++ = 0x80 | (hira & 0x3F);
                left -= 3;
            }

            last_hira = hira;
        }

    skip:
        s = next;
    }

    *out = '\0';
}


/*

enum kana_input_type
{
    KANA_TYPE_KANA = 0,
    KANA_TYPE_NON_KANA = 1,
    KANA_TYPE_ERROR = 2
};

static int handle_search_input(const char *input, char *string_output, size_t out_size, int *codes_output, int *n_codes)
{
    if (!input || !string_output || !codes_output || !n_codes || input[0] == '\0')
        return KANA_TYPE_ERROR;

    int only_kana = 1;
    const char *s = input;
    uint32_t cp;

    // Detect if input is only kana (hiragana, katakana, prolongation mark, halfwidth katakana)
    while (*s)
    {
        const char *next = utf8_decode(s, &cp);

        if (!((cp >= 0x3041 && cp <= 0x3096) || // Hiragana
              (cp >= 0x30A1 && cp <= 0x30FA) || // Katakana
              (cp == 0x30FC) ||                 // Prolongation mark
              (cp >= 0xFF66 && cp <= 0xFF9D)))  // Halfwidth katakana
        {
            only_kana = 0;
            break;
        }

        s = next;
    }

    if (only_kana)
    {
        *n_codes = kana_utf8_to_codes(input, codes_output);
        return KANA_TYPE_KANA;
    }
    else
    {
        mixed_to_hiragana(input, string_output, out_size);
        return KANA_TYPE_NON_KANA;
    }
}

*/

#endif /* KOTOBA_KANA_H */
