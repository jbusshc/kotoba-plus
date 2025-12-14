#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../kotoba-core/include/kotoba_types.h"
#include "../../kotoba-core/include/kotoba_writer.h"


#define ROUTE_JMDICT_IDX 0
#define MAX_ENTRIES 209000
char *routes[] = {
    "../assets/JMdict",
};
int routes_count = sizeof(routes) / sizeof(routes[0]);


#define BUFFER_SIZE 1024 * 16




sqlite3* open_db(const char* filename) {
    sqlite3 *db;
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

static void escape_json_string(const char* src, char* dest) {
    while (*src) {
        if (*src == '"' || *src == '\\') {
            *dest++ = '\\';
        }
        *dest++ = *src++;
    }
    *dest = '\0';
}

void create_entry_json(const entry* e, char* buffer) {
    char tmp[2048]; // buffer temporal grande para strings escapadas
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

            escape_json_string(e->k_elements[i].keb, tmp);
            strcat(buffer, tmp);
            strcat(buffer, "\"");

            if (e->k_elements[i].ke_inf_count > 0) {
                strcat(buffer, ", \"ke_inf\": [");
                for (int j = 0; j < e->k_elements[i].ke_inf_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->k_elements[i].ke_inf[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }

            if (e->k_elements[i].ke_pri_count > 0) {
                strcat(buffer, ", \"ke_pri\": [");
                for (int j = 0; j < e->k_elements[i].ke_pri_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->k_elements[i].ke_pri[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }
            strcat(buffer, "}");
        }
        strcat(buffer, "]");
    }

    // r_ele
    strcat(buffer, ", \"r_ele\": ");
    if (e->r_elements_count == 0) {
        strcat(buffer, "[]");
    } else {
        strcat(buffer, "[");
        for (int i = 0; i < e->r_elements_count; i++) {
            if (i > 0) strcat(buffer, ",");
            strcat(buffer, "{\"reb\": \"");

            escape_json_string(e->r_elements[i].reb, tmp);
            strcat(buffer, tmp);
            strcat(buffer, "\"");

            // re_nokanji
            if (e->r_elements[i].re_nokanji == true) {
                strcat(buffer, ", \"re_nokanji\": true");
            } else {
                strcat(buffer, ", \"re_nokanji\": false");
            }
            // re_restr
            if (e->r_elements[i].re_restr_count > 0) {
                strcat(buffer, ", \"re_restr\": [");
                for (int j = 0; j < e->r_elements[i].re_restr_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->r_elements[i].re_restr[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }

            // re_inf
            if (e->r_elements[i].re_inf_count > 0) {
                strcat(buffer, ", \"re_inf\": [");
                for (int j = 0; j < e->r_elements[i].re_inf_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->r_elements[i].re_inf[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }

            // re_pri
            if (e->r_elements[i].re_pri_count > 0) {
                strcat(buffer, ", \"re_pri\": [");
                for (int j = 0; j < e->r_elements[i].re_pri_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->r_elements[i].re_pri[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
            }

            strcat(buffer, "}");
        }
        strcat(buffer, "]");
    }

    // sense
    strcat(buffer, ", \"sense\": ");
    if (e->senses_count == 0) {
        strcat(buffer, "[]");
    } else {
        strcat(buffer, "[");
        for (int i = 0; i < e->senses_count; i++) {
            if (i > 0) strcat(buffer, ",");
            strcat(buffer, "{");
            int has_prev = 0;

            // stagk
            if (e->senses[i].stagk_count > 0) {
                strcat(buffer, "\"stagk\": [");
                for (int j = 0; j < e->senses[i].stagk_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].stagk[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // stagr
            if (e->senses[i].stagr_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"stagr\": [");
                for (int j = 0; j < e->senses[i].stagr_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].stagr[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // pos
            if (e->senses[i].pos_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"pos\": [");
                for (int j = 0; j < e->senses[i].pos_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].pos[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // xref
            if (e->senses[i].xref_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"xref\": [");
                for (int j = 0; j < e->senses[i].xref_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].xref[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // ant
            if (e->senses[i].ant_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"ant\": [");
                for (int j = 0; j < e->senses[i].ant_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].ant[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // field
            if (e->senses[i].field_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"field\": [");
                for (int j = 0; j < e->senses[i].field_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].field[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // misc
            if (e->senses[i].misc_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"misc\": [");
                for (int j = 0; j < e->senses[i].misc_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].misc[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // s_inf
            if (e->senses[i].s_inf_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"s_inf\": [");
                for (int j = 0; j < e->senses[i].s_inf_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].s_inf[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // lsource
            if (e->senses[i].lsource_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"lsource\": [");
                for (int j = 0; j < e->senses[i].lsource_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].lsource[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // dial
            if (e->senses[i].dial_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"dial\": [");
                for (int j = 0; j < e->senses[i].dial_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].dial[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // gloss
            if (e->senses[i].gloss_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"gloss\": [");
                for (int j = 0; j < e->senses[i].gloss_count; j++) {
                    if (j > 0) strcat(buffer, ",");
                    strcat(buffer, "\"");
                    escape_json_string(e->senses[i].gloss[j], tmp);
                    strcat(buffer, tmp);
                    strcat(buffer, "\"");
                }
                strcat(buffer, "]");
                has_prev = 1;
            }

            // example
            if (e->senses[i].examples_count > 0) {
                if (has_prev) strcat(buffer, ", ");
                strcat(buffer, "\"example\": [");

                for (int j = 0; j < e->senses[i].examples_count; j++) {
                    if (j > 0) strcat(buffer, ",");

                    strcat(buffer, "{");
                    int ep = 0;

                    if (strlen(e->senses[i].examples[j].ex_srce) > 0) {
                        escape_json_string(e->senses[i].examples[j].ex_srce, tmp);
                        strcat(buffer, "\"ex_srce\": \"");
                        strcat(buffer, tmp);
                        strcat(buffer, "\"");
                        ep = 1;
                    }

                    if (strlen(e->senses[i].examples[j].ex_text) > 0) {
                        escape_json_string(e->senses[i].examples[j].ex_text, tmp);
                        if (ep) strcat(buffer, ", ");
                        strcat(buffer, "\"ex_text\": \"");
                        strcat(buffer, tmp);
                        strcat(buffer, "\"");
                        ep = 1;
                    }

                    if (e->senses[i].examples[j].ex_sent_count > 0) {
                        if (ep) strcat(buffer, ", ");
                        strcat(buffer, "\"ex_sent\": [");
                        for (int k = 0; k < e->senses[i].examples[j].ex_sent_count; k++) {
                            if (k > 0) strcat(buffer, ",");
                            strcat(buffer, "\"");
                            escape_json_string(e->senses[i].examples[j].ex_sent[k], tmp);
                            strcat(buffer, tmp);
                            strcat(buffer, "\"");
                        }
                        strcat(buffer, "]");
                    }

                    strcat(buffer, "}");
                }

                strcat(buffer, "]");
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

    
    //sqlite3* db = open_db("kotobaplus.db");
    //if (!db) exit(1); // abortar si falla

    FILE* output_filename = fopen("dict.kotoba", "wb");
    FILE* idx_filename = fopen("dict.kotoba.idx", "wb");
    
    entry e; // Estructura estática, no puntero
    for (xmlNodePtr cur_node = root->children; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (xmlStrcmp(cur_node->name, (const xmlChar *)"entry") == 0)
            {
                memset(&e, 0, sizeof(entry));
                char buffer[BUFFER_SIZE];
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
                            if (e.k_elements_count < MAX_K_ELEMENTS )
                            {
                                xmlNodePtr k_child = NULL;
                                for (k_child = child_node->children; k_child; k_child = k_child->next)
                                {
                                    if (k_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *k_content = xmlNodeGetContent(k_child);
                                        if (xmlStrcmp(k_child->name, (const xmlChar *)"keb") == 0)
                                        {
                                            strncpy(e.k_elements[e.k_elements_count].keb, (const char *)k_content, MAX_KEB_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_inf") == 0)
                                        {
                                            strncpy(e.k_elements[e.k_elements_count].ke_inf[e.k_elements[e.k_elements_count].ke_inf_count++], (const char *)k_content, MAX_KE_INF_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_pri") == 0)
                                        {
                                            strncpy(e.k_elements[e.k_elements_count].ke_pri[e.k_elements[e.k_elements_count].ke_pri_count++], (const char *)k_content, MAX_KE_PRI_LEN);
                                        }
                                        xmlFree(k_content);
                                    }
                                }
                                e.k_elements_count++;
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"r_ele") == 0)
                        {
                            if (e.r_elements_count < MAX_R_ELEMENTS)
                            {
                                xmlNodePtr r_child = NULL;
                                for (r_child = child_node->children; r_child; r_child = r_child->next)
                                {
                                    if (r_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *r_content = xmlNodeGetContent(r_child);
                                        if (xmlStrcmp(r_child->name, (const xmlChar *)"reb") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].reb, (const char *)r_content, MAX_REB_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_nokanji") == 0)
                                        {
                                            e.r_elements[e.r_elements_count].re_nokanji = true;
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_restr") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_restr[e.r_elements[e.r_elements_count].re_restr_count++], (const char *)r_content, MAX_RE_RESTR_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_inf") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_inf[e.r_elements[e.r_elements_count].re_inf_count++], (const char *)r_content, MAX_RE_INF_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_pri") == 0)
                                        {
                                            strncpy(e.r_elements[e.r_elements_count].re_pri[e.r_elements[e.r_elements_count].re_pri_count++], (const char *)r_content, MAX_RE_PRI_LEN);
                                        }
                                        xmlFree(r_content);
                                    }
                                }
                                e.r_elements_count++;
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"sense") == 0)
                        {
                            if (e.senses_count < MAX_SENSES)
                            {
                                xmlNodePtr s_child = NULL;
                                for (s_child = child_node->children; s_child; s_child = s_child->next)
                                {
                                    if (s_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *s_content = xmlNodeGetContent(s_child);
                                        if (xmlStrcmp(s_child->name, (const xmlChar *)"stagk") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].stagk[e.senses[e.senses_count].stagk_count++], (const char *)s_content, MAX_STAGK_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"stagr") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].stagr[e.senses[e.senses_count].stagr_count++], (const char *)s_content, MAX_STAGR_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"pos") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].pos[e.senses[e.senses_count].pos_count++], (const char *)s_content, MAX_POS_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"xref") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].xref[e.senses[e.senses_count].xref_count++], (const char *)s_content, MAX_XREF_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"ant") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].ant[e.senses[e.senses_count].ant_count++], (const char *)s_content, MAX_ANT_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"field") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].field[e.senses[e.senses_count].field_count++], (const char *)s_content, MAX_FIELD_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"misc") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].misc[e.senses[e.senses_count].misc_count++], (const char *)s_content, MAX_MISC_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"s_inf") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].s_inf[e.senses[e.senses_count].s_inf_count++], (const char *)s_content, MAX_S_INF_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"lsource") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].lsource[e.senses[e.senses_count].lsource_count++], (const char *)s_content, MAX_LSOURCE_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"dial") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].dial[e.senses[e.senses_count].dial_count++], (const char *)s_content, MAX_DIAL_LEN);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"gloss") == 0)
                                        {
                                            strncpy(e.senses[e.senses_count].gloss[e.senses[e.senses_count].gloss_count++], (const char *)s_content, MAX_GLOSS_LEN);
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
                                                        strncpy(e.senses[e.senses_count].examples[e.senses[e.senses_count].examples_count].ex_srce, (const char *)e_content, MAX_EX_SRCE_LEN);
                                                    }
                                                    else if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_text") == 0)
                                                    {
                                                        strncpy(e.senses[e.senses_count].examples[e.senses[e.senses_count].examples_count].ex_text, (const char *)e_content, MAX_EX_TEXT_LEN);
                                                    }
                                                    else if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_sent") == 0)
                                                    {
                                                        if (e.senses[e.senses_count].examples[e.senses[e.senses_count].examples_count].ex_sent_count < MAX_EX_SENT)
                                                        {
                                                            strncpy(e.senses[e.senses_count].examples[e.senses[e.senses_count].examples_count].ex_sent[e.senses[e.senses_count].examples[e.senses[e.senses_count].examples_count].ex_sent_count++], (const char *)e_content, MAX_EX_SENT_LEN);
                                                        }
                                                    }
                                                    xmlFree(e_content);
                                                }
                                            }
                                            e.senses[e.senses_count].examples_count++;
                                        }
                                        xmlFree(s_content);
                                    }
                                }
                                e.senses_count++;
                            }
                        }
                        xmlFree(content);
                    }
                }
                // Optionally print or process the entry here
                // Aquí ya está la entrada completa

                write_entry_with_index(output_filename, idx_filename, &e);

                sqlite3_stmt* stmt;
                //const char* sql = "INSERT INTO entries (id, priority, entry_json) VALUES (?, ?, ?);";
                int ent_seq = e.ent_seq;
                int priority = e.priority;
                char entry_json[BUFFER_SIZE] = {0};
                //create_entry_json(&e, entry_json);
                char kanjis_plain[BUFFER_SIZE] = {0};
                char readings_plain[BUFFER_SIZE] = {0};
                char gloss_plain[BUFFER_SIZE] = {0};
                /*
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
                const char* fts_sql =
                    "INSERT INTO entry_search (entry_id, priority, content) VALUES (?, ?, ?);";

                if (sqlite3_prepare_v2(db, fts_sql, -1, &stmt, NULL) != SQLITE_OK) {
                    fprintf(stderr, "Error al preparar la consulta FTS: %s\n", sqlite3_errmsg(db));
                    continue;
                }

                // Asegurar que NO sean NULL (si vienen en NULL del parseo de JMDict)
                const char* k = (kanjis_plain[0] != '\0') ? kanjis_plain : "";
                const char* r = (readings_plain[0] != '\0') ? readings_plain : "";
                const char* g = (gloss_plain[0] != '\0') ? gloss_plain : "";


                // Construcción del campo content
                char content[BUFFER_SIZE * 3] = {0};
                snprintf(content, sizeof(content), "%s\x1F%s\x1F%s", k, r, g);

                sqlite3_bind_int(stmt, 1, ent_seq);
                sqlite3_bind_int(stmt, 2, priority); 

                // SQLITE_TRANSIENT → SQLite hace copia interna del buffer
                sqlite3_bind_text(stmt, 3, content, -1, SQLITE_TRANSIENT);

                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    fprintf(stderr, "Error al insertar en la tabla FTS: %s\n", sqlite3_errmsg(db));
                }

                sqlite3_finalize(stmt);
                */

                // Reset entry for next iteration

            }
        }
    }
    printf("Parsing completed.\n");
    //sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    xmlFreeDoc(doc);
    xmlCleanupParser();


    return 0;
}