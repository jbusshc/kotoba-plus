#ifndef KOTOBA_TYPES_H
#define KOTOBA_TYPES_H

#include <stdint.h>

/* =========================================================
 *  Kotoba+ Binary Format — Definitive Specification
 * ========================================================= */

/* ---------------------------------------------------------
 *  Archivo: magic y versión
 * --------------------------------------------------------- */

#define KOTOBA_MAGIC   0x4B4F544F  /* 'KOTO' */
#define KOTOBA_VERSION 0x0001


/* ---------------------------------------------------------
 *  Idiomas para gloss y lsource
 * --------------------------------------------------------- */
enum kotoba_lang {
    KOTOBA_LANG_EN = 0,
    KOTOBA_LANG_ES = 1,
    KOTOBA_LANG_PT = 2,
    KOTOBA_LANG_FR = 3,
    KOTOBA_LANG_DE = 4,
    KOTOBA_LANG_RU = 5,
};


/* ---------------------------------------------------------
 *  Endianness
 *  Todo el archivo es LITTLE-ENDIAN
 * --------------------------------------------------------- */

static inline uint16_t le16(uint16_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return (x >> 8) | (x << 8);
#endif
}

static inline uint32_t le32(uint32_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return  ((x >> 24) & 0x000000FF) |
            ((x >> 8)  & 0x0000FF00) |
            ((x << 8)  & 0x00FF0000) |
            ((x << 24) & 0xFF000000);
#endif
}

/* =========================================================
 *  Header del archivo (.kotoba)
 * =========================================================
 *
 *  Layout:
 *    [kotoba_file_header]
 *    [entry_index * entry_count]
 *    [entry_bin ...]
 */

typedef struct {
    uint32_t magic;        /* KOTOBA_MAGIC */
    uint16_t version;      /* KOTOBA_VERSION */
    uint16_t reserved;     /* alineación / futuro */
    uint32_t entry_count;  /* número total de entries */
} kotoba_file_header;

/* ---------------------------------------------------------
 *  Tabla de índices
 * ---------------------------------------------------------
 *
 *  entry_index[i].offset:
 *    offset ABSOLUTO desde el inicio del archivo
 *    hacia el entry_bin correspondiente
 */

typedef struct {
    uint32_t offset;
} entry_index;

/* =========================================================
 *  Header fijo de una entry
 * =========================================================
 *
 *  TODO lo que sigue después de este struct
 *  es BINARIO DINÁMICO.
 *
 *  Layout de entry:
 *
 *    [entry_bin]
 *    [k_ele blocks]
 *    [r_ele blocks]
 *    [sense blocks]
 *    [string buffer]
 */

typedef struct {
    uint32_t ent_seq;      /* ID JMdict */
    uint32_t total_size;   /* tamaño total de la entry */

    uint32_t buffer_off;   /* offset relativo al inicio de entry */
    uint32_t buffer_size;  /* tamaño del string buffer */

    uint8_t  k_count;      /* número de k_ele */
    uint8_t  r_count;      /* número de r_ele */
    uint8_t  s_count;      /* número de sense */
    uint8_t  _pad;         /* alineación */

    uint32_t k_off;        /* offset relativo a k_ele */
    uint32_t r_off;        /* offset relativo a r_ele */
    uint32_t s_off;        /* offset relativo a sense */

    int32_t  priority;     /* prioridad / ranking */
} entry_bin;

/* =========================================================
 *  LAYOUTS BINARIOS DINÁMICOS
 * =========================================================
 *
 *  NOTA FUNDAMENTAL:
 *  -----------------
 *  Los bloques siguientes NO están representados
 *  por structs C fijos.
 *
 *  Se leen SECUENCIALMENTE usando punteros.
 *
 *  Todos los offsets apuntan al STRING BUFFER
 *  de la misma entry.
 */

/* =========================================================
 *  k_ele block
 * =========================================================
 *
 *  Layout:
 *
 *    uint32 keb_off
 *    uint8  keb_len
 *
 *    uint8  ke_inf_count
 *    uint8  ke_pri_count
 *
 *    repeat ke_inf_count times:
 *        uint32 off
 *        uint8  len
 *
 *    repeat ke_pri_count times:
 *        uint32 off
 *        uint8  len
 */

/* =========================================================
 *  r_ele block
 * =========================================================
 *
 *  Layout:
 *
 *    uint32 reb_off
 *    uint8  reb_len
 *
 *    uint8  re_nokanji
 *    uint8  re_restr_count
 *    uint8  re_inf_count
 *    uint8  re_pri_count
 *
 *    repeat re_restr_count times:
 *        uint32 off
 *        uint8  len
 *
 *    repeat re_inf_count times:
 *        uint32 off
 *        uint8  len
 *
 *    repeat re_pri_count times:
 *        uint32 off
 *        uint8  len
 */

/* =========================================================
 *  sense block
 * =========================================================
 *
 *  Layout:
 *
 *    uint8 stagk_count
 *    uint8 stagr_count
 *    uint8 pos_count
 *    uint8 xref_count
 *
 *    uint8 ant_count
 *    uint8 field_count
 *    uint8 misc_count
 *    uint8 s_inf_count
 *
 *    uint8 lsource_count
 *    uint8 dial_count
 *    uint8 gloss_count
 *    uint8 examples_count
 *
 *    uint8 lang
 *
 *    repeat stagk_count times:   uint32 off + uint8  len
 *    repeat stagr_count times:   uint32 off + uint8  len
 *    repeat pos_count times:     uint32 off + uint8  len
 *    repeat xref_count times:    uint32 off + uint8  len
 *    repeat ant_count times:     uint32 off + uint8  len
 *    repeat field_count times:   uint32 off + uint8  len
 *    repeat misc_count times:    uint32 off + uint8  len
 *    repeat s_inf_count times:   uint32 off + uint16 len
 *    repeat lsource_count times: uint32 off + uint8  len
 *    repeat dial_count times:    uint32 off + uint8  len
 *    repeat gloss_count times:   uint32 off + uint16 len
 *
 *    repeat examples_count times:
 *        [example block]
 */ 

/* =========================================================
 *  example block
 * =========================================================
 *
 *  Layout:
 *
 *    uint32 ex_srce_off
 *    uint8  ex_srce_len
 *
 *    uint32 ex_text_off
 *    uint16 ex_text_len
 *
 *    uint8  ex_sent_count
 *
 *    repeat ex_sent_count times:
 *        uint32 off
 *        uint16 len
 */

/* =========================================================
 *  String buffer
 * =========================================================
 *
 *  Bloque continuo de bytes UTF-8
 *  SIN null-terminators.
 *
 *  Todos los offsets de la entry apuntan aquí.
 */


/* =========================================================
    *  Estructuras de datos estaticas en memoria, SOLO para 
       parser y generar el .kotoba
    * ========================================================= */
 
// Máximos por array de elementos
#define MAX_K_ELEMENTS   24  // max_k_elements 17
#define MAX_R_ELEMENTS   48  // max_r_elements 40
#define MAX_SENSES       32  // max_senses 26

#define MAX_KE_INF       4   // max_k_inf 2
#define MAX_KE_PRI       8   // max_k_pri 4

#define MAX_RE_RESTR     8   // max_r_restr 6
#define MAX_RE_INF       4   // max_r_inf 2
#define MAX_RE_PRI       8   // max_r_pri 6

#define MAX_STAGK        8   // max_s_tagk 4
#define MAX_STAGR        8   // max_s_tagr 4
#define MAX_POS          8   // max_s_pos 6
#define MAX_XREF         16  // max_s_xref 13
#define MAX_ANT          4   // max_s_ant 2
#define MAX_FIELD        4   // max_s_field 3
#define MAX_MISC         8   // max_s_misc 5
#define MAX_S_INF        4   // max_s_sinf 1 
#define MAX_LSOURCE      8   // max_s_lsource 4
#define MAX_DIAL         4   // max_s_dial 3
#define MAX_GLOSS        24  // max_s_gloss 15
#define MAX_EXAMPLES     8   // max_s_examples 3
#define MAX_EX_SENT      4   // max_ex_sent 2

// len
#define MAX_KEB_LEN      96   // max_keb_len 81
#define MAX_KE_INF_LEN   64   // max_ke_inf_len 46
#define MAX_KE_PRI_LEN   16   // max_ke_pri_len 5

#define MAX_REB_LEN      128  // max_reb_len 111
#define MAX_RE_RESTR_LEN 56   // max_re_restr_len 42
#define MAX_RE_INF_LEN   80   // max_re_inf_len 63
#define MAX_RE_PRI_LEN   16   // max_re_pri_len 5

#define MAX_EX_SRCE_LEN  100   // max_ex_srce_len 8
#define MAX_EX_TEXT_LEN  200  // max_ex_text_len 45
#define MAX_EX_SENT_LEN  600  // max_ex_sent_len 127

#define MAX_STAGK_LEN    100   // max_stagk_len 18
#define MAX_STAGR_LEN    100  // max_stagr_len 24
#define MAX_POS_LEN      200   // max_pos_len 71
#define MAX_XREF_LEN     300  // max_xref_len 111
#define MAX_ANT_LEN      100   // max_ant_len 42
#define MAX_FIELD_LEN    100   // max_field_len 23
#define MAX_MISC_LEN     100   // max_misc_len 44
#define MAX_S_INF_LEN    300  // max_s_inf_len 127
#define MAX_LSOURCE_LEN  100   // max_lsource_len 36
#define MAX_DIAL_LEN     100   // max_dial_len 12
#define MAX_GLOSS_LEN    300  // max_gloss_len 127


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
    bool re_nokanji; 
    char re_restr[MAX_RE_RESTR][MAX_RE_RESTR_LEN];
    int re_restr_count;
    char re_inf[MAX_RE_INF][MAX_RE_INF_LEN];
    int re_inf_count;
    char re_pri[MAX_RE_PRI][MAX_RE_PRI_LEN];
    int re_pri_count;
} r_ele;

typedef struct
{
    char ex_srce[MAX_EX_SRCE_LEN];
    char ex_text[MAX_EX_TEXT_LEN];
    char ex_sent[MAX_EX_SENT][MAX_EX_SENT_LEN];
    int ex_sent_count;
} example;

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
    example examples[MAX_EXAMPLES];
    int examples_count;
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
    int priority;
} entry;

#endif /* KOTOBA_TYPES_H */
