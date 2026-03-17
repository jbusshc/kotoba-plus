#ifndef KOTOBA_VIEWER_H
#define KOTOBA_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "loader.h"
#include "kotoba.h"
#include "dict_tokens.h"  /* para los tokenizados */

/* -------------------------
 * String binario [len][bytes]
 * ------------------------- */
typedef struct
{
    const char *ptr;
    uint8_t len;
} kotoba_str;

/* -------------------------
 * Entry access
 * ------------------------- */
KOTOBA_API const entry_bin *kotoba_entry(const kotoba_dict *d, uint32_t i);

/* -------------------------
 * k_ele
 * ------------------------- */
KOTOBA_API const k_ele_bin *kotoba_k_ele(const kotoba_dict *d, const entry_bin *e, uint32_t i);
KOTOBA_API kotoba_str kotoba_keb(const kotoba_dict *d, const k_ele_bin *k);

/* -------------------------
 * r_ele
 * ------------------------- */
KOTOBA_API const r_ele_bin *kotoba_r_ele(const kotoba_dict *d, const entry_bin *e, uint32_t i);
KOTOBA_API kotoba_str kotoba_reb(const kotoba_dict *d, const r_ele_bin *r);

/* -------------------------
 * sense
 * ------------------------- */
KOTOBA_API const sense_bin *kotoba_sense(const kotoba_dict *d, const entry_bin *e, uint32_t i);

/* Textos libres */
KOTOBA_API kotoba_str kotoba_gloss(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_s_inf(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_lsource(const kotoba_dict *d, const sense_bin *s, uint32_t i);

/* devuelven keb/reb directamente */
KOTOBA_API kotoba_str kotoba_stagk(const kotoba_dict *d, const entry_bin *e, uint8_t s_i, uint8_t k_idx);
KOTOBA_API kotoba_str kotoba_stagr(const kotoba_dict *d, const entry_bin *e, uint8_t s_i, uint8_t r_idx);

/* Tokenizados: devuelven const char* directamente */
KOTOBA_API const char* kotoba_pos(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API const char* kotoba_ke_inf(const kotoba_dict *d, const k_ele_bin *k, uint32_t i);
KOTOBA_API const char* kotoba_re_inf(const kotoba_dict *d, const r_ele_bin *r, uint32_t i);
KOTOBA_API const char* kotoba_field(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API const char* kotoba_misc(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API const char* kotoba_dial(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API const char* kotoba_ke_pri(const kotoba_dict *d, const k_ele_bin *k, uint32_t i);
KOTOBA_API const char* kotoba_re_pri(const kotoba_dict *d, const r_ele_bin *r, uint32_t i);

/* devuelven un índice */
KOTOBA_API uint32_t kotoba_xref(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API uint32_t kotoba_ant(const kotoba_dict *d, const sense_bin *s, uint32_t i);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif