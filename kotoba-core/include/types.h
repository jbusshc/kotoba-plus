#ifndef KOTOBA_TYPES_H
#define KOTOBA_TYPES_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif



#include "kotoba.h"

/* ============================================================================
 *  Idiomas (JMdict gloss languages)
 * ============================================================================
 *
 * Codificados como uint8_t.
 * El orden es estable y forma parte del ABI del binario.
 */
enum kotoba_lang
{
    KOTOBA_LANG_EN = 0,
    KOTOBA_LANG_FR,
    KOTOBA_LANG_DE,
    KOTOBA_LANG_RU,
    KOTOBA_LANG_ES,
    KOTOBA_LANG_PT,
    KOTOBA_LANG_IT,
    KOTOBA_LANG_NL,
    KOTOBA_LANG_HU,
    KOTOBA_LANG_SV,
    KOTOBA_LANG_CS,
    KOTOBA_LANG_PL,
    KOTOBA_LANG_RO,
    KOTOBA_LANG_HE,
    KOTOBA_LANG_AR,
    KOTOBA_LANG_TR,
    KOTOBA_LANG_TH,
    KOTOBA_LANG_VI,
    KOTOBA_LANG_ID,
    KOTOBA_LANG_MS,
    KOTOBA_LANG_KO,
    KOTOBA_LANG_ZH,
    KOTOBA_LANG_ZH_CN,
    KOTOBA_LANG_ZH_TW,
    KOTOBA_LANG_FA,
    KOTOBA_LANG_EO,
    KOTOBA_LANG_SLV,
    KOTOBA_LANG_UNK,
    KOTOBA_LANG_COUNT
};

/* ============================================================================
 *  Headers de archivo
 * ============================================================================
 */

#define KOTOBA_MAGIC 0x4B54422B /* 'KTB+' */
#define KOTOBA_VERSION 0x0001
#define KOTOBA_IDX_VERSION 0x0001

#define KOTOBA_FILE_BIN 1
#define KOTOBA_FILE_IDX 2

/* Header del archivo .bin (datos core) */
typedef struct
{
    uint32_t magic;
    uint32_t entry_count;
    uint16_t version;
    uint16_t type;
} kotoba_bin_header;

/* Header del archivo .idx (tabla de offsets) */
typedef struct
{
    uint32_t magic;
    uint32_t entry_count;
    uint32_t entry_stride;
    uint16_t version;
    uint16_t type;
} kotoba_idx_header;

/* ============================================================================
 *  Tabla de índices
 * ============================================================================
 *
 * Cada entrada apunta al offset absoluto dentro del .bin
 */
typedef struct
{
    uint32_t offset;
} entry_index;

/* ============================================================================
 *  Layout BINARIO mmapeable (INMUTABLE)
 * ============================================================================
 *
 * Reglas:
 *  - Sin arrays estáticos
 *  - Sin malloc
 *  - Todo acceso es offset + count
 *  - Totalmente mmapeable
 *  - ABI estable
 *
 * Strings en binario:
 *  - SIEMPRE se escriben como: [uint8_t len][bytes]
 *  - El struct guarda SOLO el offset al inicio del len
 *  - No existen *_len en structs binarios
 */

/*
 * Si *_count == 0, el correspondiente *_off es indefinido
 * y no debe ser desreferenciado.
 * 
 * Convención OFF + COUNT:
 *
 * Para todo par (X_off, X_count):
 *
 * - Si X es una colección de elementos estructurados
 *   (k_ele_bin, r_ele_bin, sense_bin),
 *   entonces X_off apunta al primer struct de un arreglo contiguo.
 *
 * - Si X es una colección de strings o tokens,
 *   entonces X_off apunta a un arreglo contiguo de uint32_t,
 *   donde cada uint32_t es un offset absoluto a un string
 *   codificado como [uint8_t len][bytes].
 *
 * - Si X_count == 0, X_off es 0 y no debe ser usado.
 * - Los offsets son siempre absolutos dentro del archivo .bin
 */

/*
    TAMAÑOS:
         entry_bin:     20 bytes
         k_ele_bin:     16 bytes
         r_ele_bin:     20 bytes
         sense_bin:     56 bytes
         kotoba_bin_header: 12 bytes
         kotoba_idx_header: 16 bytes
         entry_index:    4 bytes

*/

/* -------------------------
 * Entry (unidad principal)
 * -------------------------
 */
typedef struct
{
    int32_t ent_seq;

    uint32_t k_elements_off;
    uint32_t r_elements_off;
    uint32_t senses_off;

    uint8_t k_elements_count;
    uint8_t r_elements_count;
    uint8_t senses_count;
    uint8_t priority; /* score placeholder (0–255) */
} entry_bin;

/* -------------------------
 * Kanji element
 * -------------------------
 */
typedef struct
{
    uint32_t keb_off;
    uint32_t ke_inf_off;
    uint32_t ke_pri_off;

    uint8_t ke_inf_count;
    uint8_t ke_pri_count;
    uint16_t reserved;
} k_ele_bin;

/* -------------------------
 * Reading element
 * -------------------------
 */
typedef struct
{
    uint32_t reb_off;
    uint32_t re_restr_off;
    uint32_t re_inf_off;
    uint32_t re_pri_off;

    uint8_t re_restr_count;
    uint8_t re_inf_count;
    uint8_t re_pri_count;
    uint8_t reserved;
} r_ele_bin;

/* -------------------------
 * Sense
 * -------------------------
 */
typedef struct
{
    uint32_t stagk_off;
    uint32_t stagr_off;
    uint32_t pos_off;
    uint32_t xref_off;
    uint32_t ant_off;
    uint32_t field_off;
    uint32_t misc_off;
    uint32_t s_inf_off;
    uint32_t lsource_off;
    uint32_t dial_off;
    uint32_t gloss_off;

    uint8_t stagk_count;
    uint8_t stagr_count;
    uint8_t pos_count;
    uint8_t xref_count;
    uint8_t ant_count;
    uint8_t field_count;
    uint8_t misc_count;
    uint8_t s_inf_count;
    uint8_t lsource_count;
    uint8_t dial_count;
    uint8_t gloss_count;
    uint8_t lang; /* enum kotoba_lang */
} sense_bin;

/*
===============================================================================
EXTENSIÓN DE EJEMPLOS (DISEÑO) - NO IMPLEMENTADO AÚN
===============================================================================

- Los ejemplos NO forman parte del binario principal.
- Se relacionan vía:
      example.ent_seq -> entry.ent_seq
      example.sense_i -> índice de sense

Ventajas:
- ABI estable
- enriquecimiento incremental
- múltiples fuentes
- cero impacto en mmap del core
===============================================================================
*/

/* ============================================================================
 *  Structs estáticos, NO son parte del runtime ni deben usarse fuera
    del parser/writer.
 * ============================================================================
 */

#define MAX_K_ELEMENTS 18
#define MAX_R_ELEMENTS 41
#define MAX_SENSES 128

#define MAX_KE_INF 3
#define MAX_KE_PRI 5
#define MAX_RE_RESTR 7
#define MAX_RE_INF 3
#define MAX_RE_PRI 7

#define MAX_STAGK 8
#define MAX_STAGR 8
#define MAX_POS 12
#define MAX_XREF 26
#define MAX_ANT 4
#define MAX_FIELD 6
#define MAX_MISC 10
#define MAX_S_INF 2
#define MAX_LSOURCE 8
#define MAX_DIAL 6
#define MAX_GLOSS 160

/* ============================================================================
 *  Longitudes máximas (validadas por el parser)
 * ============================================================================
 *
 * Todas deben caber en uint8_t (<= 255)
 */

#define MAX_KEB_LEN 82
#define MAX_REB_LEN 112
#define MAX_KE_INF_LEN 47
#define MAX_KE_PRI_LEN 6
#define MAX_RE_RESTR_LEN 43
#define MAX_RE_INF_LEN 64
#define MAX_RE_PRI_LEN 8

#define MAX_STAGK_LEN 19
#define MAX_STAGR_LEN 25
#define MAX_POS_LEN 72
#define MAX_XREF_LEN 112
#define MAX_ANT_LEN 43
#define MAX_FIELD_LEN 24
#define MAX_MISC_LEN 45
#define MAX_S_INF_LEN 128
#define MAX_LSOURCE_LEN 37
#define MAX_DIAL_LEN 13
#define MAX_GLOSS_LEN 128

/* ============================================================================
 *  Structs de parsing (NO binarios, NO mmapeables)
 * ============================================================================
 */

typedef struct
{
    char keb[MAX_KEB_LEN];
    char ke_inf[MAX_KE_INF][MAX_KE_INF_LEN];
    int ke_inf_count;
    char ke_pri[MAX_KE_PRI][MAX_KE_PRI_LEN];
    int ke_pri_count;
} k_ele;

typedef struct
{
    char reb[MAX_REB_LEN];
    int re_nokanji;
    char re_restr[MAX_RE_RESTR][MAX_RE_RESTR_LEN];
    int re_restr_count;
    char re_inf[MAX_RE_INF][MAX_RE_INF_LEN];
    int re_inf_count;
    char re_pri[MAX_RE_PRI][MAX_RE_PRI_LEN];
    int re_pri_count;
} r_ele;

typedef struct
{
    char stagk[MAX_STAGK][MAX_STAGK_LEN];
    int stagk_count;
    char stagr[MAX_STAGR][MAX_STAGR_LEN];
    int stagr_count;
    char pos[MAX_POS][MAX_POS_LEN];
    int pos_count;
    char xref[MAX_XREF][MAX_XREF_LEN];
    int xref_count;
    char ant[MAX_ANT][MAX_ANT_LEN];
    int ant_count;
    char field[MAX_FIELD][MAX_FIELD_LEN];
    int field_count;
    char misc[MAX_MISC][MAX_MISC_LEN];
    int misc_count;
    char s_inf[MAX_S_INF][MAX_S_INF_LEN];
    int s_inf_count;
    char lsource[MAX_LSOURCE][MAX_LSOURCE_LEN];
    int lsource_count;
    char dial[MAX_DIAL][MAX_DIAL_LEN];
    int dial_count;
    char gloss[MAX_GLOSS][MAX_GLOSS_LEN];
    int gloss_count;

    int lang;
} sense;

typedef struct
{
    int ent_seq;
    k_ele k_elements[MAX_K_ELEMENTS];
    int k_elements_count;
    r_ele r_elements[MAX_R_ELEMENTS];
    int r_elements_count;
    sense senses[MAX_SENSES];
    int senses_count;
    uint8_t priority; /* score placeholder */
} entry;

#endif /* KOTOBA_TYPES_H */
