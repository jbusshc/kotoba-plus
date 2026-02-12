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
            if (left) {
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
