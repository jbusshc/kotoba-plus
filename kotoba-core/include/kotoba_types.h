#ifndef KOTOBA_TYPES_H
#define KOTOBA_TYPES_H

#include <stdint.h>

#include "api.h"


/* =========================================================
 *  Archivo y versión
 * ========================================================= */

#define KOTOBA_MAGIC   0x4B4F544F  /* 'KOTO' */
#define KOTOBA_VERSION 0x0001

/* =========================================================
 *  Idiomas
 * ========================================================= */

enum kotoba_lang {
    KOTOBA_LANG_EN = 0,
    KOTOBA_LANG_ES,
    KOTOBA_LANG_PT,
    KOTOBA_LANG_FR,
    KOTOBA_LANG_DE,
    KOTOBA_LANG_RU,
};

/* =========================================================
 *  Endianness helpers (LE)
 * ========================================================= */

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
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
#endif
}

/* =========================================================
 *  Header del archivo
 * ========================================================= */

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t entry_count;
    uint32_t index_off;   // offset al índice
} kotoba_file_header;

/* =========================================================
 *  Tabla de índices
 * ========================================================= */

typedef struct {
    uint32_t offset;   /* offset absoluto al entry_bin */
} entry_index;


/* =========================================================
 *  Estructura binaria de un entry (layout en archivo)
 *  Optimizada para móvil: padding mínimo, offsets dinámicos
 * ========================================================= */

typedef struct {
    uint32_t ent_seq;
    uint32_t total_size;

    uint32_t buffer_off;   // offset al buffer de datos variables
    uint32_t buffer_size;

    uint8_t  k_count;
    uint8_t  r_count;
    uint8_t  s_count;
    uint8_t  _pad;         // padding para alinear k_off a 4 bytes

    uint32_t k_off;        // offset a k_ele_bin contiguos
    uint32_t r_off;        // offset a r_ele_bin contiguos
    uint32_t s_off;        // offset a sense_bin contiguos

    int32_t  priority;
} entry_bin;


/* ------------------ k_ele_bin ------------------ */
typedef struct {
    uint32_t keb_off;       // offset al string keb
    uint8_t  ke_inf_count;  
    uint8_t  ke_pri_count;  
    uint8_t  _pad[2];       // padding para alinear a 4 bytes

    uint32_t ke_inf_off_off; // offset al array de offsets ke_inf
    uint32_t ke_pri_off_off; // offset al array de offsets ke_pri
} k_ele_bin;


/* ------------------ r_ele_bin ------------------ */
typedef struct {
    uint32_t reb_off;        // offset al string reb
    uint8_t  re_nokanji;
    uint8_t  re_restr_count;
    uint8_t  re_inf_count;
    uint8_t  re_pri_count;

    uint32_t re_restr_off_off; // offset al array de offsets re_restr
    uint32_t re_inf_off_off;   // offset al array de offsets re_inf
    uint32_t re_pri_off_off;   // offset al array de offsets re_pri
} r_ele_bin;


/* ------------------ sense_bin ------------------ */
typedef struct {
    uint8_t  stagk_count;
    uint8_t  stagr_count;
    uint8_t  pos_count;
    uint8_t  xref_count;
    uint8_t  ant_count;
    uint8_t  field_count;
    uint8_t  misc_count;
    uint8_t  s_inf_count;
    uint8_t  lsource_count;
    uint8_t  dial_count;
    uint8_t  gloss_count;
    uint8_t  examples_count;
    uint8_t  lang;
    uint8_t  _pad[3];         // padding para alinear a 4 bytes

    uint32_t stagk_off_off;   // offset al array de offsets stagk
    uint32_t stagr_off_off;
    uint32_t pos_off_off;
    uint32_t xref_off_off;
    uint32_t ant_off_off;
    uint32_t field_off_off;
    uint32_t misc_off_off;
    uint32_t s_inf_off_off;
    uint32_t lsource_off_off;
    uint32_t dial_off_off;
    uint32_t gloss_off_off;

    uint32_t examples_off_off; // offset al array de offsets de ejemplos
} sense_bin;


/* ------------------ example_bin ------------------ */
typedef struct {
    uint32_t ex_srce_off;    // offset al string fuente
    uint32_t ex_text_off;    // offset al texto
    uint8_t  ex_sent_count;
    uint8_t  _pad[3];        // padding para alinear a 4 bytes
    uint32_t ex_sent_off_off; // offset al array de offsets de frases
} example_bin;


/* =========================================================
 *  LIMITES (parser / writer)
 * ========================================================= */

#define MAX_K_ELEMENTS   18
#define MAX_R_ELEMENTS   41
#define MAX_SENSES       48

#define MAX_KE_INF       3
#define MAX_KE_PRI       5
#define MAX_RE_RESTR     7
#define MAX_RE_INF       3
#define MAX_RE_PRI       7

#define MAX_STAGK        8
#define MAX_STAGR        8
#define MAX_POS          12
#define MAX_XREF         26
#define MAX_ANT          4
#define MAX_FIELD        6
#define MAX_MISC         10
#define MAX_S_INF        2
#define MAX_LSOURCE      8
#define MAX_DIAL         6
#define MAX_GLOSS        160

#define MAX_EXAMPLES     6
#define MAX_EX_SENT      4

/* =========================================================
 *  Longitudes
 * ========================================================= */

#define MAX_KEB_LEN      82
#define MAX_REB_LEN      112
#define MAX_KE_INF_LEN  47
#define MAX_KE_PRI_LEN    6
#define MAX_RE_RESTR_LEN 43
#define MAX_RE_INF_LEN  64
#define MAX_RE_PRI_LEN    8
#define MAX_EX_SRCE_LEN  9
#define MAX_EX_TEXT_LEN  46
#define MAX_EX_SENT_LEN  128

#define MAX_STAGK_LEN    19
#define MAX_STAGR_LEN    25
#define MAX_POS_LEN      72
#define MAX_XREF_LEN     112
#define MAX_ANT_LEN      43
#define MAX_FIELD_LEN    24
#define MAX_MISC_LEN     45
#define MAX_S_INF_LEN    128
#define MAX_LSOURCE_LEN  37
#define MAX_DIAL_LEN     13
#define MAX_GLOSS_LEN    128

/* =========================================================
 *  Estructuras en memoria (parser / builder)
 * ========================================================= */

typedef struct {
    char keb[MAX_KEB_LEN];
    char ke_inf[MAX_KE_INF][MAX_S_INF_LEN];
    int  ke_inf_count;
    char ke_pri[MAX_KE_PRI][MAX_KE_PRI_LEN];
    int  ke_pri_count;
} k_ele;

typedef struct {
    char reb[MAX_REB_LEN];
    int  re_nokanji;
    char re_restr[MAX_RE_RESTR][MAX_REB_LEN];
    int  re_restr_count;
    char re_inf[MAX_RE_INF][MAX_S_INF_LEN];
    int  re_inf_count;
    char re_pri[MAX_RE_PRI][MAX_RE_PRI_LEN];
    int  re_pri_count;
} r_ele;

typedef struct {
    char ex_srce[MAX_EX_SRCE_LEN];
    char ex_text[MAX_EX_TEXT_LEN];
    char ex_sent[MAX_EX_SENT][MAX_EX_SENT_LEN];
    int  ex_sent_count;
} example;

typedef struct {
    char stagk[MAX_STAGK][MAX_STAGK_LEN];
    int  stagk_count;
    char stagr[MAX_STAGR][MAX_STAGR_LEN];
    int  stagr_count;
    char pos[MAX_POS][MAX_POS_LEN];
    int  pos_count;
    char xref[MAX_XREF][MAX_XREF_LEN];
    int  xref_count;
    char ant[MAX_ANT][MAX_ANT_LEN];
    int  ant_count;
    char field[MAX_FIELD][MAX_FIELD_LEN];
    int  field_count;
    char misc[MAX_MISC][MAX_MISC_LEN];
    int  misc_count;
    char s_inf[MAX_S_INF][MAX_S_INF_LEN];
    int  s_inf_count;
    char lsource[MAX_LSOURCE][MAX_LSOURCE_LEN];
    int  lsource_count;
    char dial[MAX_DIAL][MAX_DIAL_LEN];
    int  dial_count;
    char gloss[MAX_GLOSS][MAX_GLOSS_LEN];
    int  gloss_count;

    example examples[MAX_EXAMPLES];
    int     examples_count;

    int     lang;
} sense;

typedef struct {
    int   ent_seq;
    k_ele k_elements[MAX_K_ELEMENTS];
    int   k_elements_count;
    r_ele r_elements[MAX_R_ELEMENTS];
    int   r_elements_count;
    sense senses[MAX_SENSES];
    int   senses_count;
    int   priority;
} entry;

#endif /* KOTOBA_TYPES_H */
