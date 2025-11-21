#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#if _WIN32
#include <windows.h>
#endif

#define ROUTE_JMDICT_IDX 0
#define MAX_ENTRIES 209000
char *routes[] = {
    "../assets/JMdict_e_examp",
};
int routes_count = sizeof(routes) / sizeof(routes[0]);

#define MAX_STR_LEN 128
#define MAX_ARR_LEN 16
#define BUFFER_SIZE 1024 * 4

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


sqlite3* open_db(const char* filename) {
    sqlite3 *db;
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

void create_entry_json(const entry* e, char* buffer) {
    buffer[0] = '\0';
    strcat(buffer, "{");

    // k_ele
    strcat(buffer, "\"k_ele\": ");
    if (e->k_elements_count == 0) {
        strcat(buffer, "[]");
    } else {
        strcat(buffer, "[");
        for (int i = 0; i < e->k_elements_count; i++) {
            if (i > 0) strcat(buffer, ",");
            strcat(buffer, "{\"keb\": \"");
            strcat(buffer, e->k_elements[i].keb);
            strcat(buffer, "\"");
            if (e->k_elements[i].ke_inf_count > 0) {
                strcat(buffer, ", \"ke_inf\": [");
                for (int j = 0; j < e->k_elements[i].ke_inf_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->k_elements[i].ke_inf[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }
            if (e->k_elements[i].ke_pri_count > 0) {
                strcat(buffer, ", \"ke_pri\": [");
                for (int j = 0; j < e->k_elements[i].ke_pri_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->k_elements[i].ke_pri[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }
            strcat(buffer, "}");
        }
        strcat(buffer, "]");
    }

    strcat(buffer, ", \"r_ele\": ");
    // r_ele
    if (e->r_elements_count == 0) {
        strcat(buffer, "[]");
    } else {
        strcat(buffer, "[");
        for (int i = 0; i < e->r_elements_count; i++) {
            if (i > 0) strcat(buffer, ",");
            strcat(buffer, "{\"reb\": \"");
            strcat(buffer, e->r_elements[i].reb);
            strcat(buffer, "\"");
            if (e->r_elements[i].re_inf_count > 0) {
                strcat(buffer, ", \"re_inf\": [");
                for (int j = 0; j < e->r_elements[i].re_inf_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->r_elements[i].re_inf[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }
            if (e->r_elements[i].re_pri_count > 0) {
                strcat(buffer, ", \"re_pri\": [");
                for (int j = 0; j < e->r_elements[i].re_pri_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->r_elements[i].re_pri[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }
            strcat(buffer, "}");
        }
        strcat(buffer, "]");
    }

    strcat(buffer, ", \"sense\": ");
    // sense
    if (e->senses_count == 0) {
        strcat(buffer, "[]");
    } else {
        strcat(buffer, "[");
        for (int i = 0; i < e->senses_count; i++) {
            if (i > 0) strcat(buffer, ",");

            int has_previous_field = 0; // control para comas internas

            strcat(buffer, "{");

            // stagk
            if (e->senses[i].stagk_count > 0) {
                strcat(buffer, "\"stagk\": [");
                for (int j = 0; j < e->senses[i].stagk_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].stagk[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // stagr
            if (e->senses[i].stagr_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"stagr\": [");
                for (int j = 0; j < e->senses[i].stagr_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].stagr[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // pos
            if (e->senses[i].pos_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"pos\": [");
                for (int j = 0; j < e->senses[i].pos_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].pos[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // xref
            if (e->senses[i].xref_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"xref\": [");
                for (int j = 0; j < e->senses[i].xref_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].xref[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // ant
            if (e->senses[i].ant_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"ant\": [");
                for (int j = 0; j < e->senses[i].ant_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].ant[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // field
            if (e->senses[i].field_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"field\": [");
                for (int j = 0; j < e->senses[i].field_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].field[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // misc
            if (e->senses[i].misc_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"misc\": [");
                for (int j = 0; j < e->senses[i].misc_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].misc[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // s_inf
            if (e->senses[i].s_inf_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"s_inf\": [");
                for (int j = 0; j < e->senses[i].s_inf_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].s_inf[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // lsource
            if (e->senses[i].lsource_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"lsource\": [");
                for (int j = 0; j < e->senses[i].lsource_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].lsource[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // dial
            if (e->senses[i].dial_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"dial\": [");
                for (int j = 0; j < e->senses[i].dial_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].dial[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // gloss
            if (e->senses[i].gloss_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"gloss\": [");
                for (int j = 0; j < e->senses[i].gloss_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    strcat(buffer, e->senses[i].gloss[j]);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_previous_field = 1;
            }

            // example (solo primer ejemplo, según original)
            if (e->senses[i].examples[0].ex_srce[0] != '\0' || e->senses[i].examples[0].ex_text[0] != '\0' || e->senses[i].examples[0].ex_sent_count > 0) {
                if (has_previous_field) strcat(buffer, ", ");
                strcat(buffer, "\"example\": {");
                int has_prev_example = 0;
                if (e->senses[i].examples[0].ex_srce[0] != '\0') {
                    strcat(buffer, "\"ex_srce\": \"");
                    strcat(buffer, e->senses[i].examples[0].ex_srce);
                    strcat(buffer, "\"");
                    has_prev_example = 1;
                }
                if (e->senses[i].examples[0].ex_text[0] != '\0') {
                    if (has_prev_example) strcat(buffer, ", ");
                    strcat(buffer, "\"ex_text\": \"");
                    strcat(buffer, e->senses[i].examples[0].ex_text);
                    strcat(buffer, "\"");
                    has_prev_example = 1;
                }
                if (e->senses[i].examples[0].ex_sent_count > 0) {
                    if (has_prev_example) strcat(buffer, ", ");
                    strcat(buffer, "\"ex_sent\": [");
                    for (int j = 0; j < e->senses[i].examples[0].ex_sent_count; j++) {
                        if (j > 0) strcat(buffer, ",");
                        strcat(buffer, "\"");
                        strcat(buffer, e->senses[i].examples[0].ex_sent[j]);
                        strcat(buffer, "\"");
                    }
                    strcat(buffer, "]");
                }
                strcat(buffer, "}");
            }

            strcat(buffer, "}");
        }
        strcat(buffer, "]");
    }

    strcat(buffer, "}");
}

void create_kanjis_plain_text(const entry* e, char* buffer) {
    if (e->k_elements_count == 0) {
        buffer[0] = '\0';
        return; // No kanji elements to process
    }
    buffer[0] = '\0';
    for (int i = 0; i < e->k_elements_count; i++) {
        if (i > 0) strcat(buffer, " ");
        strcat(buffer, e->k_elements[i].keb);
    }
}

void create_readings_plain_text(const entry* e, char* buffer) {
    if (e->r_elements_count == 0) {
        buffer[0] = '\0';
        return; // No reading elements to process
    }
    buffer[0] = '\0';
    for (int i = 0; i < e->r_elements_count; i++) {
        if (i > 0) strcat(buffer, " ");
        strcat(buffer, e->r_elements[i].reb);
        if (e->r_elements[i].re_nokanji[0]) {
            strcat(buffer, " (");
            strcat(buffer, e->r_elements[i].re_nokanji);
            strcat(buffer, ")");
        }
    }
}

void create_gloss_plain_text(const entry* e, char* buffer) {
    if (e->senses_count == 0) {
        buffer[0] = '\0';
        return; // No senses to process
    }
    buffer[0] = '\0';
    for (int i = 0; i < e->senses_count; i++) {
        for (int j = 0; j < e->senses[i].gloss_count; j++) {
            if (j > 0 || (i > 0 && e->senses[i - 1].gloss_count > 0)) strcat(buffer, ", ");
            strcat(buffer, e->senses[i].gloss[j]);
        }
        if (i < e->senses_count - 1) strcat(buffer, "; ");
    }
}

int main() {
    #if _WIN32
    system("chcp 65001"); // Change code page to UTF-8 for Windows
    #endif

    xmlDocPtr doc = xmlReadFile(routes[ROUTE_JMDICT_IDX], NULL, 0);
    if (doc == NULL)
    {
        fprintf(stderr, "Failed to parse %s\n", routes[ROUTE_JMDICT_IDX]);
        return 1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == NULL)
    {
        fprintf(stderr, "Empty XML document\n");
        xmlFreeDoc(doc);
        return 1;
    }

    sqlite3* db = open_db("tango.db");
    if (!db) exit(1); // abortar si falla
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    for (xmlNodePtr cur_node = root->children; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (xmlStrcmp(cur_node->name, (const xmlChar *)"entry") == 0)
            {
                entry e = {0};
                xmlNodePtr child_node = NULL;
                for (child_node = cur_node->children; child_node; child_node = child_node->next)
                {
                    if (child_node->type == XML_ELEMENT_NODE)
                    {
                        xmlChar *content = xmlNodeGetContent(child_node);
                        if (xmlStrcmp(child_node->name, (const xmlChar *)"ent_seq") == 0)
                        {
                            e.ent_seq = atoi((const char *)content);
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"k_ele") == 0)
                        {
                            if (e.k_elements_count < MAX_ARR_LEN)
                            {
                                xmlNodePtr k_child = NULL;
                                for (k_child = child_node->children; k_child; k_child = k_child->next)
                                {
                                    if (k_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *k_content = xmlNodeGetContent(k_child);
                                        if (xmlStrcmp(k_child->name, (const xmlChar *)"keb") == 0)
                                        {
                                            strncpy(e.k_elements[e.k_elements_count].keb, (const char *)k_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_inf") == 0)
                                        {
                                            strncpy(e.k_elements[e.k_elements_count].ke_inf[e.k_elements[e.k_elements_count].ke_inf_count++], (const char *)k_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_pri") == 0)
                                        {
                                            strncpy(e.k_elements[e.k_elements_count].ke_pri[e.k_elements[e.k_elements_count].ke_pri_count++], (const char *)k_content, MAX_STR_LEN);
                                        }
                                    }
                                }
                                e.k_elements_count++;
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"r_ele") == 0)
                        {
                            if (e.r_elements_count < MAX_ARR_LEN)
                            {
                                xmlNodePtr r_child = NULL;
                                for (r_child = child_node->children; r_child; r_child = r_child->next)
                                {
                                    if (r_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *r_content = xmlNodeGetContent(r_child);
                                        if (xmlStrcmp(r_child->name, (const xmlChar *)"reb") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].reb, (const char *)r_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_nokanji") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_nokanji, (const char *)r_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_restr") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_restr[e.r_elements[e.r_elements_count].re_restr_count++], (const char *)r_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_inf") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_inf[e.r_elements[e.r_elements_count].re_inf_count++], (const char *)r_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_pri") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_pri[e.r_elements[e.r_elements_count].re_pri_count++], (const char *)r_content, MAX_STR_LEN);
                                        }
                                    }
                                }
                                e.r_elements_count++;
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"sense") == 0)
                        {
                            if (e.senses_count < MAX_ARR_LEN)
                            {
                                xmlNodePtr s_child = NULL;
                                for (s_child = child_node->children; s_child; s_child = s_child->next)
                                {
                                    if (s_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *s_content = xmlNodeGetContent(s_child);
                                        if (xmlStrcmp(s_child->name, (const xmlChar *)"stagk") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].stagk[e.senses[e.senses_count].stagk_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"stagr") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].stagr[e.senses[e.senses_count].stagr_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"pos") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].pos[e.senses[e.senses_count].pos_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"xref") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].xref[e.senses[e.senses_count].xref_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"ant") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].ant[e.senses[e.senses_count].ant_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"field") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].field[e.senses[e.senses_count].field_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"misc") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].misc[e.senses[e.senses_count].misc_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"s_inf") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].s_inf[e.senses[e.senses_count].s_inf_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"lsource") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].lsource[e.senses[e.senses_count].lsource_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"dial") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].dial[e.senses[e.senses_count].dial_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"gloss") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].gloss[e.senses[e.senses_count].gloss_count++], (const char *)s_content, MAX_STR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"example") == 0)
                                        {
                                            xmlNodePtr e_child = NULL;
                                            for (e_child = s_child->children; e_child; e_child = e_child->next)
                                            {
                                                if (e_child->type == XML_ELEMENT_NODE)
                                                {
                                                    xmlChar *e_content = xmlNodeGetContent(e_child);
                                                    if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_srce") == 0)
                                                    {
                                                        strncpy(e.senses[e.senses_count].examples[0].ex_srce, (const char *)e_content, MAX_STR_LEN);
                                                    }
                                                    else if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_text") == 0)
                                                    {
                                                        strncpy(e.senses[e.senses_count].examples[0].ex_text, (const char *)e_content, MAX_STR_LEN);
                                                    }
                                                    else if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_sent") == 0)
                                                    {
                                                        if (e.senses[e.senses_count].examples[0].ex_sent_count < MAX_ARR_LEN)
                                                        {
                                                            strncpy(e.senses[e.senses_count].examples[0].ex_sent[e.senses[e.senses_count].examples[0].ex_sent_count++], (const char *)e_content, MAX_STR_LEN);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                e.senses_count++;
                            }
                        }
                    }
                }
                // Optionally print or process the entry here
                // Aquí ya está la entrada completa

                sqlite3_stmt* stmt;
                const char* sql = "INSERT INTO entries (id, priority, entry_json) VALUES (?, ?, ?);";
                int ent_seq = e.ent_seq;
                int priority = e.priority;
                char entry_json[BUFFER_SIZE] = {0};
                create_entry_json(&e, entry_json);
                char kanjis_plain[BUFFER_SIZE] = {0};
                char readings_plain[BUFFER_SIZE] = {0};
                char gloss_plain[BUFFER_SIZE] = {0};
                create_kanjis_plain_text(&e, kanjis_plain);
                create_readings_plain_text(&e, readings_plain);
                create_gloss_plain_text(&e, gloss_plain);
                if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                    fprintf(stderr, "Error al preparar la consulta: %s\n", sqlite3_errmsg(db));
                    continue;
                }
                sqlite3_bind_int(stmt, 1, ent_seq);
                sqlite3_bind_int(stmt, 2, priority);
                sqlite3_bind_text(stmt, 3, entry_json, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    fprintf(stderr, "Error al insertar la entrada: %s\n", sqlite3_errmsg(db));
                }
                sqlite3_finalize(stmt);
                // Insert into FTS table
                const char* fts_sql = "INSERT INTO entry_search (entry_id, priority, kanji, reading, gloss) VALUES (?, ?, ?, ?, ?);";
                if (sqlite3_prepare_v2(db, fts_sql, -1, &stmt, NULL) != SQLITE_OK) {
                    fprintf(stderr, "Error al preparar la consulta FTS: %s\n", sqlite3_errmsg(db));
                    continue;
                }
                sqlite3_bind_int(stmt, 1, ent_seq);
                sqlite3_bind_int(stmt, 2, priority);
                sqlite3_bind_text(stmt, 3, kanjis_plain, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, readings_plain, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 5, gloss_plain, -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    fprintf(stderr, "Error al insertar en la tabla FTS: %s\n", sqlite3_errmsg(db));
                }
                sqlite3_finalize(stmt);
                // Reset entry for next iteration
                e = (entry){0}; // Reset entry structure
            }
        }
    }
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return 0;
}