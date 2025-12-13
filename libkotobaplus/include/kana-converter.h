#ifndef KANA_CONVERTER_H
#define KANA_CONVERTER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <locale.h>

#define MAX_OUTPUT 512

typedef struct {
    const char *romaji;
    const char *hiragana;
    const char *katakana;
} KanaMap;

extern KanaMap kana_map[];

void to_lower(char *s);
void change_l_to_r(char *s);
int is_romaji_char(char c);
void convert_romaji_buffer(const char *buffer, char *output, int is_katakana);
void romaji_mixed_to_kana(const char *input, char *output, int is_katakana);


#endif // KANA_CONVERTER_H