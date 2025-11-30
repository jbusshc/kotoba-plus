#ifndef LIBTANGO_H
#define LIBTANGO_H

#ifdef _WIN32
  #ifdef LIBTANGO_EXPORTS
    #define LIBTANGO __declspec(dllexport)
  #else
    #define LIBTANGO __declspec(dllimport)
  #endif
#else
  #define LIBTANGO
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct TangoDB TangoDB;
typedef struct srs_stats srs_stats;

#ifndef __cplusplus
    #include <stdint.h>
    #ifndef uint64_t
        #define uint64_t unsigned long long int
    #endif
    #include <stdbool.h>
#endif

LIBTANGO TangoDB* tango_db_open(const char* db_path);
LIBTANGO void tango_db_close(TangoDB* db);
LIBTANGO void tango_db_warmup(TangoDB* db);


typedef struct {
    int ent_seq;
    const char* kanjis;
    const char* readings;
    const char* glosses;
    int priority;
} TangoSearchResult;

// Define el tipo de función callback
typedef void (*TangoSearchResultCallback)(const TangoSearchResult*, void* userdata);

// Función con callback
LIBTANGO void tango_db_text_search(TangoDB* db, const char* termino, TangoSearchResultCallback callback, void* userdata);

// Máximos por array de elementos
#define MAX_K_ELEMENTS   24  // max_k_elements 16 + margen
#define MAX_R_ELEMENTS   48  // max_r_elements 16 + margen
#define MAX_SENSES       32  // max_senses 16 + margen

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

#define MAX_EX_SRCE_LEN  16   // max_ex_srce_len 8
#define MAX_EX_TEXT_LEN  64   // max_ex_text_len 45
#define MAX_EX_SENT_LEN  160  // max_ex_sent_len 127

#define MAX_STAGK_LEN    24   // max_stagk_len 18
#define MAX_STAGR_LEN    32   // max_stagr_len 24
#define MAX_POS_LEN      96   // max_pos_len 71
#define MAX_XREF_LEN     128  // max_xref_len 111
#define MAX_ANT_LEN      48   // max_ant_len 42
#define MAX_FIELD_LEN    32   // max_field_len 23
#define MAX_MISC_LEN     64   // max_misc_len 44
#define MAX_S_INF_LEN    160  // max_s_inf_len 127
#define MAX_LSOURCE_LEN  48   // max_lsource_len 36
#define MAX_DIAL_LEN     16   // max_dial_len 12
#define MAX_GLOSS_LEN    160  // max_gloss_len 127

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


typedef void (*TangoEntryCallback)(const entry*, void* userdata);
LIBTANGO void tango_db_id_search(TangoDB* db, int id, entry* e, TangoEntryCallback callback, void* userdata);

LIBTANGO void tango_update_srs_review(TangoDB* db, int entry_id, int quality);


typedef struct {
    int entry_id;
    int64_t last_review;
    int interval;        
    double ease_factor;
    int repetitions;
    int64_t due_date;
} srs_review;

typedef struct {
    int ent_seq; // Entry sequence number
    srs_review review_data;
    entry entry_data;
} review_card;

#define BUFFER_LIMIT 16

typedef struct {
    review_card cards[BUFFER_LIMIT];
    int count;
    int start_index;
    int current_index; // Index of the current card being reviewed
} review_card_buffer;

LIBTANGO void tango_review_card_buffer_init(TangoDB* db, review_card_buffer* buffer);

LIBTANGO void tango_review_card_buffer_fill(TangoDB* db, review_card_buffer* buffer, int cards_to_add);

LIBTANGO review_card* tango_review_card_buffer_get_current(review_card_buffer* buffer);
LIBTANGO void tango_review_card_buffer_advance(review_card_buffer* buffer);
LIBTANGO void tango_review_card_buffer_undo(review_card_buffer* buffer);

LIBTANGO int tango_get_pending_review_cards(TangoDB* db);

LIBTANGO void tango_update_srs_stats(TangoDB* db);
LIBTANGO int tango_get_total_cards(TangoDB* db);
LIBTANGO int tango_get_due_review_count(TangoDB* db);
LIBTANGO void tango_review_card_buffer_clear(review_card_buffer* buffer);
LIBTANGO srs_stats* tango_get_srs_stats(TangoDB* db);

#ifdef __cplusplus
}
#endif

#endif /* LIBTANGO_H */
