#include <stdio.h>
#include <stdlib.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../kotoba-core/include/kotoba_types.h"
#include "../../kotoba-core/include/kotoba_writer.h"
#include "../../kotoba-core/include/kotoba_loader.h"
#include "../../kotoba-core/include/kotoba_dict.h"


const char* dict_path = "dict.kotoba";
const char* idx_path = "dict.kotoba.idx";

#define ROUTE_JMDICT_IDX 0
#define MAX_ENTRIES 209000
char *routes[] = {
    "../assets/JMdict",
};
int routes_count = sizeof(routes) / sizeof(routes[0]);


void parse_jmdict(xmlNodePtr root, FILE* output_filename, FILE* idx_filename, uint64_t* entry_count) {
    entry e;
    for (xmlNodePtr cur_node = root->children; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (xmlStrcmp(cur_node->name, (const xmlChar *)"entry") == 0)
            {
                memset(&e, 0, sizeof(entry));
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
                write_entry_with_index(output_filename, idx_filename, &e);
                (*entry_count)++;
            }
        }
    }
}

void create_kotoba_dict(){ 

    printf("Starting parsing JMdict...\n");
    uint64_t entry_count = 0;
    xmlDocPtr doc = xmlReadFile(routes[ROUTE_JMDICT_IDX], NULL, 0);
    if (doc == NULL)
    {
        fprintf(stderr, "Failed to parse %s\n", routes[ROUTE_JMDICT_IDX]);
        return;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == NULL)
    {
        fprintf(stderr, "Empty XML document\n");
        xmlFreeDoc(doc);
        return;
    }

    FILE* output_filename = fopen(dict_path, "wb");
    FILE* idx_filename = fopen(idx_path, "wb");

    kotoba_write_header(output_filename, NULL);
    kotoba_write_header(idx_filename, NULL);

    parse_jmdict(root, output_filename, idx_filename, &entry_count);
    printf("Parsing completed.\n");
    // Update headers with correct entry count
    kotoba_bin_header bin_header = {
        .magic = KOTOBA_MAGIC,
        .version = KOTOBA_VERSION,
        .reserved = 0,
        .entry_count = (uint32_t)entry_count
    };
    kotoba_write_header(output_filename, &bin_header);
    kotoba_idx_header idx_header = {
        .magic = KOTOBA_MAGIC,
        .version = KOTOBA_IDX_VERSION,
        .reserved = 0,
        .entry_count = (uint32_t)entry_count
    };
    kotoba_write_header(idx_filename, &idx_header);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    
}

int main() {
    #if _WIN32
    system("chcp 65001"); // Change code page to UTF-8 for Windows
    #endif

    // writing stuff
    create_kotoba_dict();

    /*
    kotoba_dict dict;
    if (kotoba_dict_open(&dict, dict_path, idx_path) != 0) {
        fprintf(stderr, "Failed to open dictionary files.\n");
        return 1;
    }
        */

    return 0;
}