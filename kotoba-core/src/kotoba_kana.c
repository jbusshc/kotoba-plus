#include "kotoba_kana.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ============================================================================
 * Tabla romaji → kana (TU CÓDIGO, intacto)
 * ============================================================================
 */

const KanaMap kana_map[] = {
    {"kya", "きゃ", "キャ"}, {"kyu", "きゅ", "キュ"}, {"kyo", "きょ", "キョ"},
    {"sha", "しゃ", "シャ"}, {"shu", "しゅ", "シュ"}, {"sho", "しょ", "ショ"},
    {"cha", "ちゃ", "チャ"}, {"chu", "ちゅ", "チュ"}, {"cho", "ちょ", "チョ"},
    {"nya", "にゃ", "ニャ"}, {"nyu", "にゅ", "ニュ"}, {"nyo", "にょ", "ニョ"},
    {"hya", "ひゃ", "ヒャ"}, {"hyu", "ひゅ", "ヒュ"}, {"hyo", "ひょ", "ヒョ"},
    {"mya", "みゃ", "ミャ"}, {"myu", "みゅ", "ミュ"}, {"myo", "みょ", "ミョ"},
    {"rya", "りゃ", "リャ"}, {"ryu", "りゅ", "リュ"}, {"ryo", "りょ", "リョ"},
    {"gya", "ぎゃ", "ギャ"}, {"gyu", "ぎゅ", "ギュ"}, {"gyo", "ぎょ", "ギョ"},
    {"bya", "びゃ", "ビャ"}, {"byu", "びゅ", "ビュ"}, {"byo", "びょ", "ビョ"},
    {"pya", "ぴゃ", "ピャ"}, {"pyu", "ぴゅ", "ピュ"}, {"pyo", "ぴょ", "ピョ"},
    {"shi", "し", "シ"}, {"chi", "ち", "チ"}, {"tsu", "つ", "ツ"},
    {"fu", "ふ", "フ"}, {"ji", "じ", "ジ"},
    {"a", "あ", "ア"}, {"i", "い", "イ"}, {"u", "う", "ウ"},
    {"e", "え", "エ"}, {"o", "お", "オ"},
    {"ka", "か", "カ"}, {"ki", "き", "キ"}, {"ku", "く", "ク"},
    {"ke", "け", "ケ"}, {"ko", "こ", "コ"},
    {"sa", "さ", "サ"}, {"su", "す", "ス"}, {"se", "せ", "セ"}, {"so", "そ", "ソ"},
    {"ta", "た", "タ"}, {"te", "て", "テ"}, {"to", "と", "ト"},
    {"na", "な", "ナ"}, {"ni", "に", "ニ"}, {"nu", "ぬ", "ヌ"},
    {"ne", "ね", "ネ"}, {"no", "の", "ノ"},
    {"ha", "は", "ハ"}, {"hi", "ひ", "ヒ"}, {"he", "へ", "ヘ"}, {"ho", "ほ", "ホ"},
    {"ma", "ま", "マ"}, {"mi", "み", "ミ"}, {"mu", "む", "ム"},
    {"me", "め", "メ"}, {"mo", "も", "モ"},
    {"ya", "や", "ヤ"}, {"yu", "ゆ", "ユ"}, {"yo", "よ", "ヨ"},
    {"ra", "ら", "ラ"}, {"ri", "り", "リ"}, {"ru", "る", "ル"},
    {"re", "れ", "レ"}, {"ro", "ろ", "ロ"},
    {"wa", "わ", "ワ"}, {"wo", "を", "ヲ"},
    {"ga", "が", "ガ"}, {"gi", "ぎ", "ギ"}, {"gu", "ぐ", "グ"},
    {"ge", "げ", "ゲ"}, {"go", "ご", "ゴ"},
    {"za", "ざ", "ザ"}, {"ze", "ぜ", "ゼ"}, {"zo", "ぞ", "ゾ"},
    {"da", "だ", "ダ"}, {"de", "で", "デ"}, {"do", "ど", "ド"},
    {"ba", "ば", "バ"}, {"bi", "び", "ビ"}, {"bu", "ぶ", "ブ"},
    {"be", "べ", "ベ"}, {"bo", "ぼ", "ボ"},
    {"pa", "ぱ", "パ"}, {"pi", "ぴ", "ピ"}, {"pu", "ぷ", "プ"},
    {"pe", "ぺ", "ペ"}, {"po", "ぽ", "ポ"},
    {"n", "ん", "ン"},
    {NULL, NULL, NULL}
};

/* ============================================================================
 * Romaji → Kana (igual a tu lógica)
 * ============================================================================
 */

static int is_romaji_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void convert_romaji_buffer(const char *buffer,
                                  char *output,
                                  int is_katakana)
{
    size_t i = 0;

    while (buffer[i]) {
        int matched = 0;

        /* sokuon */
        if (isalpha(buffer[i]) &&
            buffer[i] == buffer[i + 1] &&
            buffer[i] != 'n') {
            strcat(output, is_katakana ? "ッ" : "っ");
            i++;
            continue;
        }

        for (int k = 0; kana_map[k].romaji; k++) {
            size_t len = strlen(kana_map[k].romaji);
            if (strncmp(&buffer[i], kana_map[k].romaji, len) == 0) {
                strcat(output,
                       is_katakana ? kana_map[k].katakana
                                   : kana_map[k].hiragana);
                i += len;
                matched = 1;
                break;
            }
        }

        if (!matched) {
            strncat(output, &buffer[i], 1);
            i++;
        }
    }
}

void romaji_mixed_to_kana(const char *input,
                          char *output,
                          int is_katakana)
{
    char romaji_buf[64] = {0};
    output[0] = '\0';

    size_t len = strlen(input);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)input[i];

        if (is_romaji_char(c)) {
            size_t rlen = strlen(romaji_buf);
            if (rlen + 1 < sizeof(romaji_buf)) {
                romaji_buf[rlen] = (char)tolower(c);
                romaji_buf[rlen + 1] = '\0';
            }
        } else {
            if (romaji_buf[0]) {
                convert_romaji_buffer(romaji_buf, output, is_katakana);
                romaji_buf[0] = '\0';
            }

            int bytes = 1;
            if ((c & 0xF8) == 0xF0) bytes = 4;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xE0) == 0xC0) bytes = 2;

            char tmp[5] = {0};
            memcpy(tmp, &input[i], bytes);
            strcat(output, tmp);
            i += bytes - 1;
        }
    }

    if (romaji_buf[0])
        convert_romaji_buffer(romaji_buf, output, is_katakana);
}

/* ============================================================================
 * Unicode utils
 * ============================================================================
 */

const char* utf8_decode(const char *s, uint32_t *cp)
{
    unsigned char c = (unsigned char)s[0];

    if (c < 0x80) {
        *cp = c;
        return s + 1;
    }
    if ((c & 0xE0) == 0xC0) {
        *cp = ((c & 0x1F) << 6) | (s[1] & 0x3F);
        return s + 2;
    }
    if ((c & 0xF0) == 0xE0) {
        *cp = ((c & 0x0F) << 12) |
              ((s[1] & 0x3F) << 6) |
              (s[2] & 0x3F);
        return s + 3;
    }
    if ((c & 0xF8) == 0xF0) {
        *cp = ((c & 0x07) << 18) |
              ((s[1] & 0x3F) << 12) |
              ((s[2] & 0x3F) << 6) |
              (s[3] & 0x3F);
        return s + 4;
    }

    *cp = 0xFFFD;
    return s + 1;
}

/* ============================================================================
 * Kana normalization
 * ============================================================================
 */

uint32_t kana_to_hiragana(uint32_t cp)
{
    if (cp >= 0x30A1 && cp <= 0x30F6)
        return cp - 0x60;
    return cp;
}

uint32_t normalize_kana(uint32_t cp)
{
    cp = kana_to_hiragana(cp);

    /* prolongación */
    if (cp == 0x30FC)
        return 0x3046; /* う */

    return cp;
}

/* ============================================================================
 * Kana → code compacto (DAT)
 * ============================================================================
 */


const kana_code_map kana_codes[] = {
    /* vocales */
    {0x3041,  1}, /* ぁ */
    {0x3042,  2}, /* あ */
    {0x3043,  3}, /* ぃ */
    {0x3044,  4}, /* い */
    {0x3045,  5}, /* ぅ */
    {0x3046,  6}, /* う */
    {0x3047,  7}, /* ぇ */
    {0x3048,  8}, /* え */
    {0x3049,  9}, /* ぉ */
    {0x304A, 10}, /* お */

    /* k */
    {0x304B, 11}, /* か */
    {0x304C, 12}, /* が */
    {0x304D, 13}, /* き */
    {0x304E, 14}, /* ぎ */
    {0x304F, 15}, /* く */
    {0x3050, 16}, /* ぐ */
    {0x3051, 17}, /* け */
    {0x3052, 18}, /* げ */
    {0x3053, 19}, /* こ */
    {0x3054, 20}, /* ご */

    /* s */
    {0x3055, 21}, /* さ */
    {0x3056, 22}, /* ざ */
    {0x3057, 23}, /* し */
    {0x3058, 24}, /* じ */
    {0x3059, 25}, /* す */
    {0x305A, 26}, /* ず */
    {0x305B, 27}, /* せ */
    {0x305C, 28}, /* ぜ */
    {0x305D, 29}, /* そ */
    {0x305E, 30}, /* ぞ */

    /* t */
    {0x305F, 31}, /* た */
    {0x3060, 32}, /* だ */
    {0x3061, 33}, /* ち */
    {0x3062, 34}, /* ぢ */
    {0x3063, 35}, /* っ */
    {0x3064, 36}, /* つ */
    {0x3065, 37}, /* づ */
    {0x3066, 38}, /* て */
    {0x3067, 39}, /* で */
    {0x3068, 40}, /* と */
    {0x3069, 41}, /* ど */

    /* n */
    {0x306A, 42}, /* な */
    {0x306B, 43}, /* に */
    {0x306C, 44}, /* ぬ */
    {0x306D, 45}, /* ね */
    {0x306E, 46}, /* の */

    /* h */
    {0x306F, 47}, /* は */
    {0x3070, 48}, /* ば */
    {0x3071, 49}, /* ぱ */
    {0x3072, 50}, /* ひ */
    {0x3073, 51}, /* び */
    {0x3074, 52}, /* ぴ */
    {0x3075, 53}, /* ふ */
    {0x3076, 54}, /* ぶ */
    {0x3077, 55}, /* ぷ */
    {0x3078, 56}, /* へ */
    {0x3079, 57}, /* べ */
    {0x307A, 58}, /* ぺ */
    {0x307B, 59}, /* ほ */
    {0x307C, 60}, /* ぼ */
    {0x307D, 61}, /* ぽ */

    /* m */
    {0x307E, 62}, /* ま */
    {0x307F, 63}, /* み */
    {0x3080, 64}, /* む */
    {0x3081, 65}, /* め */
    {0x3082, 66}, /* も */

    /* y */
    {0x3083, 67}, /* ゃ */
    {0x3084, 68}, /* や */
    {0x3085, 69}, /* ゅ */
    {0x3086, 70}, /* ゆ */
    {0x3087, 71}, /* ょ */
    {0x3088, 72}, /* よ */

    /* r */
    {0x3089, 73}, /* ら */
    {0x308A, 74}, /* り */
    {0x308B, 75}, /* る */
    {0x308C, 76}, /* れ */
    {0x308D, 77}, /* ろ */

    /* w */
    {0x308E, 78}, /* ゎ */
    {0x308F, 79}, /* わ */
    {0x3090, 80}, /* ゐ */
    {0x3091, 81}, /* ゑ */
    {0x3092, 82}, /* を */

    /* n */
    {0x3093, 83}, /* ん */

    {0, 0}
};


int kana_code(uint32_t cp)
{
    for (int i = 0; kana_codes[i].cp; i++) {
        if (kana_codes[i].cp == cp)
            return kana_codes[i].code;
    }
    return -1;
}

/* ============================================================================
 * UTF-8 → DAT codes
 * ============================================================================
 */

int kana_utf8_to_codes(const char *utf8, int *out_codes)
{
    int n = 0;
    uint32_t cp;

    while (*utf8) {
        utf8 = utf8_decode(utf8, &cp);
        cp = normalize_kana(cp);

        int code = kana_code(cp);
        if (code > 0 && n < MAX_KANA_CODES)
            out_codes[n++] = code;
    }
    return n;
}
