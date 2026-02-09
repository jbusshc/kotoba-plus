#include "kana.h"

void mixed_to_hiragana(const char *input, char *output, size_t out_size)
{
    KanaStream ks;
    kana_stream_init(&ks);

    const char *s = input;
    char *out = output;
    size_t left = out_size - 1;

    while (*s && left)
    {
        unsigned char c = (unsigned char)*s;

        /* romaji separator */
        if (c == '\'')
        {
            /* special case: trailing 'n' */
            if (ks.carry_len == 1 && ks.carry[0] == 'n')
            {
                emit_utf8(0x3093, &out, &left); /* ん */
                ks.carry_len = 0;
            }
            s++;
            continue;
        }


        /* ASCII romaji */
        if (isalpha(c))
        {
            kana_stream_feed(&ks, (char)tolower(c), &out, &left);
            s++;
            continue;
        }

        /* UTF-8 kana path */
        uint32_t cp;
        const char *next = utf8_decode(s, &cp);
        cp = normalize_kana(cp);
        emit_utf8(cp, &out, &left);
        s = next;
    }

    kana_stream_flush(&ks, &out, &left);
    *out = '\0';
}
