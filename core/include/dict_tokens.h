#ifndef DICT_TOKENS_H
#define DICT_TOKENS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kotoba.h"

// DEFINE JMDICT METADATA AS TOKENS FOR EFFICIENT STORAGE IN THE BINARY FORMAT (UINT8_T)

enum g_type {
    LIT = 1, // literal meaning
    FIG = 2, // figurative meaning
    TM = 3,  // trademark
    EXPL = 4, // explanatory meaning
    DESCR = 5 // descriptive meaning
};


enum ls_type { // LS_TYPE AND LS_WASEI MERGED TO | OPERATION, AS THEY ARE NOT MUTUALLY EXCLUSIVE
    LS_TYPE_FULL = 0, // default, mutualmente exclusivo con LS_TYPE_PART, como solo se marca part, la ausencia de part indica full
    LS_TYPE_PART = 1, // zero-width non-joiner as a marker for part of name, if not present it's marked as full
    LS_WASEI = 2 
};
 

/* DIAL */
extern const char* dial[];    
extern const int dial_count;
/* FIELD */
extern const char* field[];   
extern const int field_count;  
/* KE_INF */
extern const char* ke_inf[];   
extern const int ke_inf_count;  
/* MISC */
extern const char* misc[];
extern const int misc_count;
/* POS */
extern const char* pos[];
extern const int pos_count;
/* RE_INF */
extern const char* re_inf[];
extern const int re_inf_count;

/* KE_PRI AND RE_PRI */
extern const char* pri_types[]; // KE_PRI and RE_PRI share the same set of priority types
extern const int pri_types_count;
/* KE_PRI */
extern const char** ke_pri;
extern const int ke_pri_count;
/* RE_PRI */
extern const char** re_pri;
extern const int re_pri_count;

KOTOBA_API int dial_token_index(const char *dial_str);
KOTOBA_API const char* dial_token_str(int index);
KOTOBA_API int field_token_index(const char *field_str);
KOTOBA_API const char* field_token_str(int index);
KOTOBA_API int ke_inf_token_index(const char *ke_inf_str);
KOTOBA_API const char* ke_inf_token_str(int index);
KOTOBA_API int misc_token_index(const char *misc_str);
KOTOBA_API const char* misc_token_str(int index);
KOTOBA_API int pos_token_index(const char *pos_str);
KOTOBA_API const char* pos_token_str(int index);
KOTOBA_API int re_inf_token_index(const char *re_inf_str);
KOTOBA_API const char* re_inf_token_str(int index);
KOTOBA_API int ke_pri_token_index(const char *ke_pri_str);
KOTOBA_API const char* ke_pri_token_str(int index);
KOTOBA_API int re_pri_token_index(const char *re_pri_str);
KOTOBA_API const char* re_pri_token_str(int index);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif