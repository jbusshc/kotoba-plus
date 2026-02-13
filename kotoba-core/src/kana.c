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
    // --- A Row (あ) ---
    case 0x3041: // ぁ (small a)
    case 0x3042: // あ
    case 0x304B: // か
    case 0x304C: // が
    case 0x3055: // さ
    case 0x3056: // ざ
    case 0x305F: // た
    case 0x3060: // だ
    case 0x306A: // な
    case 0x306F: // は
    case 0x3070: // ば
    case 0x3071: // ぱ
    case 0x307E: // ま
    case 0x3083: // ゃ (small ya)
    case 0x3084: // や
    case 0x3089: // ら
    case 0x308F: // わ
        return 0x3042;

    // --- I Row (い) ---
    case 0x3043: // ぃ (small i)
    case 0x3044: // い
    case 0x304D: // き
    case 0x304E: // ぎ
    case 0x3057: // し
    case 0x3058: // じ
    case 0x3061: // ち
    case 0x3062: // ぢ
    case 0x306B: // に
    case 0x3072: // ひ
    case 0x3073: // び
    case 0x3074: // ぴ
    case 0x307F: // み
    case 0x308A: // り
        return 0x3044;

    // --- U Row (う) ---
    case 0x3045: // ぅ (small u)
    case 0x3046: // う
    case 0x304F: // く
    case 0x3050: // ぐ
    case 0x3059: // す
    case 0x305A: // ず
    case 0x3064: // つ
    case 0x3065: // づ
    case 0x306C: // ぬ
    case 0x3075: // ふ
    case 0x3076: // ぶ
    case 0x3077: // ぷ
    case 0x3080: // む
    case 0x3085: // ゅ (small yu)
    case 0x3086: // ゆ
    case 0x308B: // る
    case 0x3094: // ゔ (vu)
        return 0x3046;

    // --- E Row (え) ---
    case 0x3047: // ぇ (small e)
    case 0x3048: // え
    case 0x3051: // け
    case 0x3052: // げ
    case 0x305B: // せ
    case 0x305C: // ぜ
    case 0x3066: // て
    case 0x3067: // で
    case 0x306D: // ね
    case 0x3078: // へ
    case 0x3079: // べ
    case 0x307A: // ぺ
    case 0x3081: // め
    case 0x308C: // れ
        return 0x3048;

    // --- O Row (お) ---
    case 0x3049: // ぉ (small o)
    case 0x304A: // お
    case 0x3053: // こ
    case 0x3054: // ご
    case 0x305D: // そ
    case 0x305E: // ぞ
    case 0x3068: // と
    case 0x3069: // ど
    case 0x306E: // の
    case 0x307B: // ほ
    case 0x307C: // ぼ
    case 0x307D: // ぽ
    case 0x3082: // ま
    case 0x3087: // ょ (small yo)
    case 0x3088: // よ
    case 0x308D: // ろ
    case 0x3092: // を (wo)
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
                emit_utf8(cp, &out, &left);     // mantener primera sílaba
                emit_utf8(0x30FC, &out, &left); // añadir ー
                s = peek;
                continue;
            }
        }

        emit_utf8(cp, &out, &left);
        s = next;
    }

    *out = '\0';
}
