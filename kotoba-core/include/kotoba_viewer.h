#ifndef KOTOBA_VIEWER_H
#define KOTOBA_VIEWER_H

#include "kotoba_types.h"
#include "kotoba_loader.h"
#include "api.h"

/* -------------------------
 * String binario
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
KOTOBA_API kotoba_str kotoba_gloss(const kotoba_dict *d, const sense_bin *s, uint32_t i);

KOTOBA_API kotoba_str kotoba_stagk(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_stagr(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_pos(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_xref(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_ant(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_field(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_misc(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_s_inf(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_lsource(const kotoba_dict *d, const sense_bin *s, uint32_t i);
KOTOBA_API kotoba_str kotoba_dial(const kotoba_dict *d, const sense_bin *s, uint32_t i);




#endif
