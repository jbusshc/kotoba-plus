#include "kana-converter.h"

KanaMap kana_map[] = {
    {"kya", "きゃ", "キャ"}, {"kyu", "きゅ", "キュ"}, {"kyo", "きょ", "キョ"},
    {"sha", "しゃ", "シャ"}, {"shu", "しゅ", "シュ"}, {"sho", "しょ", "ショ"},
    {"cha", "ちゃ", "チャ"}, {"chu", "ちゅ", "チュ"}, {"cho", "ちょ", "チョ"},
    {"nya", "にゃ", "ニャ"}, {"nyu", "にゅ", "ニュ"}, {"nyo", "にょ", "ニョ"},
    {"hya", "ひゃ", "ヒャ"}, {"hyu", "ひゅ", "ヒュ"}, {"hyo", "ひょ", "ヒョ"},
    {"mya", "みゃ", "ミャ"}, {"myu", "みゅ", "ミュ"}, {"myo", "みょ", "ミョ"},
    {"rya", "りゃ", "リャ"}, {"ryu", "りゅ", "リュ"}, {"ryo", "りょ", "リョ"},
    {"gya", "ぎゃ", "ギャ"}, {"gyu", "ぎゅ", "ギュ"}, {"gyo", "ぎょ", "ギョ"},
    {"bya", "びゃ", "ビャ"}, {"byu", "びゅ", "ビュ"}, {"byo", "びょ", "ビョ"},
    {"pya", "ぴゃ", "ピャ"}, {"pyu", "ぴゅ", "ピュ"}, {"pyo", "ぴょ", "ピュ"},
    {"shi", "し", "シ"}, {"chi", "ち", "チ"}, {"tsu", "つ", "ツ"}, {"fu", "ふ", "フ"},
    {"ji", "じ", "ジ"}, {"zu", "ず", "ズ"}, {"du", "づ", "ヅ"},
    {"a", "あ", "ア"}, {"i", "い", "イ"}, {"u", "う", "ウ"}, {"e", "え", "エ"}, {"o", "お", "オ"},
    {"ka", "か", "カ"}, {"ki", "き", "キ"}, {"ku", "く", "ク"}, {"ke", "け", "ケ"}, {"ko", "こ", "コ"},
    {"sa", "さ", "サ"}, {"su", "す", "ス"}, {"se", "せ", "セ"}, {"so", "そ", "ソ"},
    {"ta", "た", "タ"}, {"te", "て", "テ"}, {"to", "と", "ト"},
    {"na", "な", "ナ"}, {"ni", "に", "ニ"}, {"nu", "ぬ", "ヌ"}, {"ne", "ね", "ネ"}, {"no", "の", "ノ"},
    {"ha", "は", "ハ"}, {"hi", "ひ", "ヒ"}, {"he", "へ", "ヘ"}, {"ho", "ほ", "ホ"},
    {"ma", "ま", "マ"}, {"mi", "み", "ミ"}, {"mu", "む", "ム"}, {"me", "め", "メ"}, {"mo", "も", "モ"},
    {"ya", "や", "ヤ"}, {"yu", "ゆ", "ユ"}, {"yo", "よ", "ヨ"},
    {"ra", "ら", "ラ"}, {"ri", "り", "リ"}, {"ru", "る", "ル"}, {"re", "れ", "レ"}, {"ro", "ろ", "ロ"},
    {"wa", "わ", "ワ"}, {"wo", "を", "ヲ"},
    {"ga", "が", "ガ"}, {"gi", "ぎ", "ギ"}, {"gu", "ぐ", "グ"}, {"ge", "げ", "ゲ"}, {"go", "ご", "ゴ"},
    {"za", "ざ", "ザ"}, {"ji", "じ", "ジ"}, {"zu", "ず", "ズ"}, {"ze", "ぜ", "ゼ"}, {"zo", "ぞ", "ゾ"},
    {"da", "だ", "ダ"}, {"di", "ぢ", "ヂ"}, {"du", "づ", "ヅ"}, {"de", "で", "デ"}, {"do", "ど", "ド"},
    {"ba", "ば", "バ"}, {"bi", "び", "ビ"}, {"bu", "ぶ", "ブ"}, {"be", "べ", "ベ"}, {"bo", "ぼ", "ボ"},
    {"pa", "ぱ", "パ"}, {"pi", "ぴ", "ピ"}, {"pu", "ぷ", "プ"}, {"pe", "ぺ", "ペ"}, {"po", "ぽ", "ポ"},
    {"n", "ん", "ン"},
    {NULL, NULL, NULL}
};

void to_lower(char *s) {
    for (; *s; ++s) *s = tolower(*s);
}

void change_l_to_r(char *s) {
    for (; *s; ++s) {
        if (*s == 'l') *s = 'r';
        else if (*s == 'L') *s = 'R';
    }
}

int is_romaji_char(char c) {
    return isalpha(c);
}

void convert_romaji_buffer(const char *buffer, char *output, int is_katakana) {
    size_t i = 0;
    while (buffer[i]) {
        int matched = 0;

        // Sokuon
        if (isalpha(buffer[i]) && buffer[i] == buffer[i+1] && buffer[i] != 'n') {
            strcat(output, is_katakana ? "ッ" : "っ");
            i++;
            continue;
        }

        for (int k = 0; kana_map[k].romaji != NULL; k++) {
            size_t len = strlen(kana_map[k].romaji);
            if (strncmp(&buffer[i], kana_map[k].romaji, len) == 0) {
                strcat(output, is_katakana ? kana_map[k].katakana : kana_map[k].hiragana);
                i += len;
                matched = 1;
                break;
            }
        }

        if (!matched) {
            strncat(output, &buffer[i], 1); // Copia carácter no reconocido
            i++;
        }
    }
}


void romaji_mixed_to_kana(const char *input, char *output, int is_katakana) {
    char romaji_buf[64] = "";
    output[0] = '\0';

    size_t len = strlen(input);
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)input[i];

        // Si es ASCII romaji
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            size_t rlen = strlen(romaji_buf);
            if (rlen < sizeof(romaji_buf) - 1) {
                romaji_buf[rlen] = tolower(c);
                romaji_buf[rlen + 1] = '\0';
            }
        } else {
            // Convertir buffer romaji si está lleno
            if (strlen(romaji_buf) > 0) {
                convert_romaji_buffer(romaji_buf, output, is_katakana);
                romaji_buf[0] = '\0';
            }

            // Detectar longitud del carácter UTF-8
            int bytes = 1;
            if ((c & 0xF8) == 0xF0) bytes = 4;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xE0) == 0xC0) bytes = 2;

            // Evitar desbordes
            if (i + bytes <= len) {
                char utf8[5] = {0};
                strncpy(utf8, &input[i], bytes);
                strcat(output, utf8);
                i += bytes - 1;
            }
        }
    }

    // Procesar cualquier romaji restante
    if (strlen(romaji_buf) > 0) {
        convert_romaji_buffer(romaji_buf, output, is_katakana);
    }
}

/*
void print_bytes(const char *str) {
    size_t len = strlen(str);
    printf("Bytes (hex): ");
    for (size_t i = 0; i < len; ++i) {
        printf("%02X ", (unsigned char)str[i]);
    }
    printf("\n");

    printf("Bytes (dec): ");
    for (size_t i = 0; i < len; ++i) {
        printf("%d ", (unsigned char)str[i]);
    }
    printf("\n");
}
*/