#ifndef KOTOBA_KANA_H
#define KOTOBA_KANA_H

#include <stdint.h>
#include <stddef.h>
#include "api.h"

#define MAX_KANA_OUTPUT 512
#define MAX_KANA_CODES  128

/* ============================================================================
 * Romaji → Kana (lo que ya usabas)
 * ============================================================================
 */

typedef struct {
    const char *romaji;
    const char *hiragana;
    const char *katakana;
} KanaMap;

extern const KanaMap kana_map[];

KOTOBA_API void romaji_mixed_to_kana(const char *input,
                          char *output,
                          int is_katakana);

/* ============================================================================
 * Unicode / Kana utils (NUEVO)
 * ============================================================================
 */
 
typedef struct {
    uint32_t cp;
    uint8_t  code;
} kana_code_map;

extern const kana_code_map kana_codes[];

/* UTF-8 decode: devuelve puntero al siguiente char */
KOTOBA_API const char* utf8_decode(const char *s, uint32_t *cp);

/* Normalización kana */
KOTOBA_API uint32_t kana_to_hiragana(uint32_t cp);
KOTOBA_API uint32_t normalize_kana(uint32_t cp);
/* Kana → code compacto (1..N), -1 si no es kana */
KOTOBA_API int kana_code(uint32_t cp);

/* UTF-8 → codes[] para DAT */
KOTOBA_API int kana_utf8_to_codes(const char *utf8, int *out_codes);

#endif /* KOTOBA_KANA_H */
