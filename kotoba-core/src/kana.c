#include "kana.h"

void mixed_to_hiragana(const TrieContext *ctx, const char *input, char *output, size_t out_size)
{

    const char *s = input;
    char *out = output;
    size_t left = out_size - 1;

    while (*s && left)
    {
        unsigned char c = (unsigned char)*s;

        /* --- Romaji path --- */
        if (isalpha(c))
        {
            /* SOKUON */
            if (isalpha((unsigned char)s[1]) &&
                tolower(s[0]) == tolower(s[1]) &&
                is_consonant(tolower(s[0])) &&
                tolower(s[0]) != 'n')
            {
                emit_utf8(0x3063, &out, &left); /* っ */
                s++;
                continue;
            }

            uint16_t cur = 0;
            uint16_t last = 0;
            int last_len = 0;

            const char *p = s;

            while (*p && isalpha((unsigned char)*p))
            {
                int k = tolower(*p) - 'a';
                if (k < 0 || k >= 26)
                    break;

                if (!ctx->trie[cur].child[k])
                    break;

                cur = ctx->trie[cur].child[k];

                if (ctx->trie[cur].out_len)
                {
                    last = cur;
                    last_len = (int)(p - s + 1);
                }

                p++;
            }

            if (last)
            {
                /* emitir hiragana (convertimos si estaba en katakana) */
                for (int i = 0; i < ctx->trie[last].out_len; i++)
                {
                    uint32_t cp = (i == 0)
                                      ? ctx->trie[last].out1
                                      : ctx->trie[last].out2;

                    /* katakana → hiragana */
                    if (cp >= 0x30A1 && cp <= 0x30F6)
                        cp -= 0x60;

                    emit_utf8(cp, &out, &left);
                }

                s += last_len;
                continue;
            }

            /* no match → copiar literal */
            if (left)
            {
                *out++ = *s++;
                left--;
            }
        }

        /* --- Separador romaji ' --- */
        if (c == '\'')
        {
            s++;
            continue;
        }

        /* --- UTF8 path (kana/kanji/etc) --- */
        uint32_t cp;
        const char *next = utf8_decode(s, &cp);

        /* Katakana → Hiragana */
        if (cp >= 0x30A1 && cp <= 0x30F6)
            cp -= 0x60;
        else if (cp == 0x30FC)
            cp = 0x30FC; // mantener como está

        emit_utf8(cp, &out, &left);

        s = next;
    }

    *out = '\0';
}

static inline uint32_t hiragana_vowel_(uint32_t cp)
{
    switch (cp)
    {
        // A row
        case 0x3042: case 0x304B: case 0x3055: case 0x305F:
        case 0x306A: case 0x306F: case 0x307E: case 0x3084:
        case 0x3089: case 0x308F:
            return 0x3042;

        // I row
        case 0x3044: case 0x304D: case 0x3057: case 0x3061:
        case 0x306B: case 0x3072: case 0x307F: case 0x308A:
            return 0x3044;

        // U row
        case 0x3046: case 0x304F: case 0x3059: case 0x3064:
        case 0x306C: case 0x3075: case 0x3080: case 0x3086:
        case 0x308B:
            return 0x3046;

        // E row
        case 0x3048: case 0x3051: case 0x305B: case 0x3066:
        case 0x306D: case 0x3078: case 0x3081: case 0x308C:
            return 0x3048;

        // O row
        case 0x304A: case 0x3053: case 0x305D: case 0x3068:
        case 0x306E: case 0x307B: case 0x3082: case 0x3088:
        case 0x308D:
            return 0x304A;

        default:
            return 0;
    }
}


void vowel_prolongation_mark(const char *input, char *output, size_t out_size)
{
    const char *s = input;
    char *out = output;
    size_t left = out_size - 1;

    while (*s && left)
    {
        uint32_t cp;
        const char *next = utf8_decode(s, &cp);

        uint32_t v1 = hiragana_vowel_(cp);
        if (v1)
        {
            uint32_t next_cp;
            const char *peek = utf8_decode(next, &next_cp);
            uint32_t v2 = hiragana_vowel_(next_cp);

            if (v2 && v1 == v2)
            {
                emit_utf8(cp, &out, &left);      // mantener primera sílaba
                emit_utf8(0x30FC, &out, &left);  // añadir ー
                s = peek;
                continue;
            }
        }


        emit_utf8(cp, &out, &left);
        s = next;
    }

    *out = '\0';
}
