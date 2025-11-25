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

// Nueva función con callback
LIBTANGO void tango_db_text_search(TangoDB* db, const char* termino, TangoSearchResultCallback callback, void* userdata);


#define MAX_STR_LEN 128
#define MAX_ARR_LEN 16

// <!ELEMENT k_ele (keb, ke_inf*, ke_pri*)>
typedef struct
{
    char keb[MAX_STR_LEN];                 // Kanji reading
    char ke_inf[MAX_ARR_LEN][MAX_STR_LEN]; // Kanji information (optional)
    int ke_inf_count;                      // Count of kanji information
    char ke_pri[MAX_ARR_LEN][MAX_STR_LEN]; // Kanji priority (optional)
    int ke_pri_count;                      // Count of kanji priority
} k_ele;

// <!ELEMENT r_ele (reb, re_nokanji?, re_restr*, re_inf*, re_pri*)>

typedef struct
{
    char reb[MAX_STR_LEN];                   // Reading
    char re_nokanji[MAX_STR_LEN];            // No kanji reading (optional)
    char re_restr[MAX_ARR_LEN][MAX_STR_LEN]; // Reading restrictions (optional)
    int re_restr_count;                      // Count of reading restrictions
    char re_inf[MAX_ARR_LEN][MAX_STR_LEN];   // Reading information (optional)
    int re_inf_count;                        // Count of reading information
    char re_pri[MAX_ARR_LEN][MAX_STR_LEN];   // Reading priority (optional)
    int re_pri_count;                        // Count of reading priority
} r_ele;

// <!ELEMENT example (ex_srce,ex_text,ex_sent+)>
typedef struct
{
    char ex_srce[MAX_STR_LEN];              // Source of the example
    char ex_text[MAX_STR_LEN];              // Example text
    char ex_sent[MAX_ARR_LEN][MAX_STR_LEN]; // Example sentence
    int ex_sent_count;                      // Count of example sentences
} example;

// <!ELEMENT sense (stagk*, stagr*, pos*, xref*, ant*, field*, misc*, s_inf*, lsource*, dial*, gloss*, example*)>
typedef struct
{
    char stagk[MAX_ARR_LEN][MAX_STR_LEN];   // Kanji tags (optional)
    int stagk_count;                        // Count of kanji tags
    char stagr[MAX_ARR_LEN][MAX_STR_LEN];   // Reading tags (optional)
    int stagr_count;                        // Count of reading tags
    char pos[MAX_ARR_LEN][MAX_STR_LEN];     // Part of speech (optional)
    int pos_count;                          // Count of parts of speech
    char xref[MAX_ARR_LEN][MAX_STR_LEN];    // Cross-references (optional)
    int xref_count;                         // Count of cross-references
    char ant[MAX_ARR_LEN][MAX_STR_LEN];     // Antonyms (optional)
    int ant_count;                          // Count of antonyms
    char field[MAX_ARR_LEN][MAX_STR_LEN];   // Field tags (optional)
    int field_count;                        // Count of field tags
    char misc[MAX_ARR_LEN][MAX_STR_LEN];    // Miscellaneous information (optional)
    int misc_count;                         // Count of miscellaneous information
    char s_inf[MAX_ARR_LEN][MAX_STR_LEN];   // Sense information (optional)
    int s_inf_count;                        // Count of sense information
    char lsource[MAX_ARR_LEN][MAX_STR_LEN]; // Source of the sense (optional)
    int lsource_count;                      // Count of sources
    char dial[MAX_ARR_LEN][MAX_STR_LEN];    // Dialect information (optional)
    int dial_count;                         // Count of dialects
    char gloss[MAX_ARR_LEN][MAX_STR_LEN];   // Glosses (translations) for the sense
    int gloss_count;                        // Count of glosses
    example examples[MAX_ARR_LEN];          // Examples for the sense
    int examples_count;                     // Count of examples
} sense;

// <!ELEMENT entry (ent_seq, k_ele*, r_ele*, sense+)>
typedef struct
{
    int ent_seq;                   // Entry sequence number
    k_ele k_elements[MAX_ARR_LEN]; // Kanji elements (optional)
    int k_elements_count;          // Count of kanji elements
    r_ele r_elements[MAX_ARR_LEN]; // Reading elements (optional)
    int r_elements_count;          // Count of reading elements
    sense senses[MAX_ARR_LEN];     // Senses associated with the entry
    int senses_count;              // Count of senses
    int priority;              // Priority of the entry (not in XML, but can be derived)
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
