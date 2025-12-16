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

const char *dict_path = "dict.kotoba";
const char *idx_path = "dict.kotoba.idx";

#define ROUTE_JMDICT_IDX 0
#define MAX_ENTRIES 209000
char *routes[] = {
    "../assets/JMdict",
};
int routes_count = sizeof(routes) / sizeof(routes[0]);

void parse_jmdict(xmlNodePtr root, FILE *output_filename, FILE *idx_filename, uint64_t *entry_count)
{
    entry *e = (entry *)malloc(sizeof(entry));
    if (!e) {
        fprintf(stderr, "Failed to allocate memory for entry\n");
        return;
    }
    for (xmlNodePtr cur_node = root->children; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (xmlStrcmp(cur_node->name, (const xmlChar *)"entry") == 0)
            {
                // Initialize entry fields without memset
                e->k_elements_count = 0;
                e->r_elements_count = 0;
                e->senses_count = 0;
                e->ent_seq = 0;
                // Initialize k_elements
                for (int i = 0; i < MAX_K_ELEMENTS; i++) {
                    e->k_elements[i].keb[0] = '\0';
                    e->k_elements[i].ke_inf_count = 0;
                    for (int j = 0; j < MAX_KE_INF; j++) {
                        e->k_elements[i].ke_inf[j][0] = '\0';
                    }
                    e->k_elements[i].ke_pri_count = 0;
                    for (int j = 0; j < MAX_KE_PRI; j++) {
                        e->k_elements[i].ke_pri[j][0] = '\0';
                    }
                }
                // Initialize r_elements
                for (int i = 0; i < MAX_R_ELEMENTS; i++) {
                    e->r_elements[i].reb[0] = '\0';
                    e->r_elements[i].re_nokanji = false;
                    e->r_elements[i].re_restr_count = 0;
                    for (int j = 0; j < MAX_RE_RESTR; j++) {
                        e->r_elements[i].re_restr[j][0] = '\0';
                    }
                    e->r_elements[i].re_inf_count = 0;
                    for (int j = 0; j < MAX_RE_INF; j++) {
                        e->r_elements[i].re_inf[j][0] = '\0';
                    }
                    e->r_elements[i].re_pri_count = 0;
                    for (int j = 0; j < MAX_RE_PRI; j++) {
                        e->r_elements[i].re_pri[j][0] = '\0';
                    }
                }
                // Initialize senses
                for (int i = 0; i < MAX_SENSES; i++) {
                    e->senses[i].lang = KOTOBA_LANG_EN;
                    e->senses[i].stagk_count = 0;
                    for (int j = 0; j < MAX_STAGK; j++) {
                        e->senses[i].stagk[j][0] = '\0';
                    }
                    e->senses[i].stagr_count = 0;
                    for (int j = 0; j < MAX_STAGR; j++) {
                        e->senses[i].stagr[j][0] = '\0';
                    }
                    e->senses[i].pos_count = 0;
                    for (int j = 0; j < MAX_POS; j++) {
                        e->senses[i].pos[j][0] = '\0';
                    }
                    e->senses[i].xref_count = 0;
                    for (int j = 0; j < MAX_XREF; j++) {
                        e->senses[i].xref[j][0] = '\0';
                    }
                    e->senses[i].ant_count = 0;
                    for (int j = 0; j < MAX_ANT; j++) {
                        e->senses[i].ant[j][0] = '\0';
                    }
                    e->senses[i].field_count = 0;
                    for (int j = 0; j < MAX_FIELD; j++) {
                        e->senses[i].field[j][0] = '\0';
                    }
                    e->senses[i].misc_count = 0;
                    for (int j = 0; j < MAX_MISC; j++) {
                        e->senses[i].misc[j][0] = '\0';
                    }
                    e->senses[i].s_inf_count = 0;
                    for (int j = 0; j < MAX_S_INF; j++) {
                        e->senses[i].s_inf[j][0] = '\0';
                    }
                    e->senses[i].lsource_count = 0;
                    for (int j = 0; j < MAX_LSOURCE; j++) {
                        e->senses[i].lsource[j][0] = '\0';
                    }
                    e->senses[i].dial_count = 0;
                    for (int j = 0; j < MAX_DIAL; j++) {
                        e->senses[i].dial[j][0] = '\0';
                    }
                    e->senses[i].gloss_count = 0;
                    for (int j = 0; j < MAX_GLOSS; j++) {
                        e->senses[i].gloss[j][0] = '\0';
                    }
                    e->senses[i].examples_count = 0;
                    for (int j = 0; j < MAX_EXAMPLES; j++) {
                        e->senses[i].examples[j].ex_srce[0] = '\0';
                        e->senses[i].examples[j].ex_text[0] = '\0';
                        e->senses[i].examples[j].ex_sent_count = 0;
                        for (int k = 0; k < MAX_EX_SENT; k++) {
                            e->senses[i].examples[j].ex_sent[k][0] = '\0';
                        }
                    }
                }
                
                xmlNodePtr child_node = NULL;
                for (child_node = cur_node->children; child_node; child_node = child_node->next)
                {
                    if (child_node->type == XML_ELEMENT_NODE)
                    {
                        xmlChar *content = xmlNodeGetContent(child_node);
                        if (xmlStrcmp(child_node->name, (const xmlChar *)"ent_seq") == 0)
                        {
                            e->ent_seq = atoi((const char *)content);
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"k_ele") == 0)
                        {
                            if (e->k_elements_count < MAX_K_ELEMENTS)
                            {
                                xmlNodePtr k_child = NULL;
                                for (k_child = child_node->children; k_child; k_child = k_child->next)
                                {
                                    if (k_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *k_content = xmlNodeGetContent(k_child);
                                        if (xmlStrcmp(k_child->name, (const xmlChar *)"keb") == 0)
                                        {
                                            strncpy(e->k_elements[e->k_elements_count].keb, (const char *)k_content, MAX_KEB_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_inf") == 0)
                                        {
                                            if (e->k_elements[e->k_elements_count].ke_inf_count < MAX_KE_INF)
                                            {
                                                strncpy(e->k_elements[e->k_elements_count].ke_inf[e->k_elements[e->k_elements_count].ke_inf_count++], (const char *)k_content, MAX_KE_INF_LEN);
                                            }
                                            else
                                            {
                                                printf("Max ke_inf reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_pri") == 0)
                                        {
                                            if (e->k_elements[e->k_elements_count].ke_pri_count < MAX_KE_PRI)
                                            {
                                                strncpy(e->k_elements[e->k_elements_count].ke_pri[e->k_elements[e->k_elements_count].ke_pri_count++], (const char *)k_content, MAX_KE_PRI_LEN);
                                            }
                                            else
                                            {
                                                printf("Max ke_pri reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        xmlFree(k_content);
                                    }
                                }
                                e->k_elements_count++;
                            }
                            else
                            {
                                printf("Max k_elements reached for ent_seq: %d\n", e->ent_seq);
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"r_ele") == 0)
                        {
                            if (e->r_elements_count < MAX_R_ELEMENTS)
                            {
                                xmlNodePtr r_child = NULL;
                                for (r_child = child_node->children; r_child; r_child = r_child->next)
                                {
                                    if (r_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *r_content = xmlNodeGetContent(r_child);
                                        if (xmlStrcmp(r_child->name, (const xmlChar *)"reb") == 0)
                                        {
                                            strncpy(e->r_elements[e->r_elements_count].reb, (const char *)r_content, MAX_REB_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_nokanji") == 0)
                                        {
                                            e->r_elements[e->r_elements_count].re_nokanji = true;
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_restr") == 0)
                                        {
                                            if (e->r_elements[e->r_elements_count].re_restr_count < MAX_RE_RESTR)
                                            {
                                                strncpy(e->r_elements[e->r_elements_count].re_restr[e->r_elements[e->r_elements_count].re_restr_count++], (const char *)r_content, MAX_RE_RESTR_LEN);
                                            }
                                            else
                                            {
                                                printf("Max re_restr reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_inf") == 0)
                                        {
                                            if (e->r_elements[e->r_elements_count].re_inf_count < MAX_RE_INF)
                                            {
                                                strncpy(e->r_elements[e->r_elements_count].re_inf[e->r_elements[e->r_elements_count].re_inf_count++], (const char *)r_content, MAX_RE_INF_LEN);
                                            }
                                            else
                                            {
                                                printf("Max re_inf reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_pri") == 0)
                                        {
                                            if (e->r_elements[e->r_elements_count].re_pri_count < MAX_RE_PRI)
                                            {
                                                strncpy(e->r_elements[e->r_elements_count].re_pri[e->r_elements[e->r_elements_count].re_pri_count++], (const char *)r_content, MAX_RE_PRI_LEN);
                                            }
                                            else
                                            {
                                                printf("Max re_pri reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        xmlFree(r_content);
                                    }
                                }
                                e->r_elements_count++;
                            }
                            else
                            {
                                printf("Max r_elements reached for ent_seq: %d\n", e->ent_seq);
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"sense") == 0)
                        {

                            if (e->senses_count < MAX_SENSES)
                            {
                                e->senses[e->senses_count].lang = KOTOBA_LANG_EN; // default JMdict
                                xmlNodePtr s_child = NULL;
                                for (s_child = child_node->children; s_child; s_child = s_child->next)
                                {
                                    if (s_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *s_content = xmlNodeGetContent(s_child);
                                        if (xmlStrcmp(s_child->name, (const xmlChar *)"stagk") == 0)
                                        {
                                            if (e->senses[e->senses_count].stagk_count < MAX_STAGK)
                                            {
                                                strncpy(e->senses[e->senses_count].stagk[e->senses[e->senses_count].stagk_count++], (const char *)s_content, MAX_STAGK_LEN);
                                            }
                                            else
                                            {
                                                printf("Max stagk reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"stagr") == 0)
                                        {
                                            if (e->senses[e->senses_count].stagr_count < MAX_STAGR)
                                            {
                                                strncpy(e->senses[e->senses_count].stagr[e->senses[e->senses_count].stagr_count++], (const char *)s_content, MAX_STAGR_LEN);
                                            }
                                            else
                                            {
                                                printf("Max stagr reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"pos") == 0)
                                        {
                                            if (e->senses[e->senses_count].pos_count < MAX_POS)
                                            {
                                                strncpy(e->senses[e->senses_count].pos[e->senses[e->senses_count].pos_count++], (const char *)s_content, MAX_POS_LEN);
                                            }
                                            else
                                            {
                                                printf("Max pos reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"xref") == 0)
                                        {
                                            if (e->senses[e->senses_count].xref_count < MAX_XREF)
                                            {
                                                strncpy(e->senses[e->senses_count].xref[e->senses[e->senses_count].xref_count++], (const char *)s_content, MAX_XREF_LEN);
                                            }
                                            else
                                            {
                                                printf("Max xref reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"ant") == 0)
                                        {
                                            if (e->senses[e->senses_count].ant_count < MAX_ANT)
                                            {
                                                strncpy(e->senses[e->senses_count].ant[e->senses[e->senses_count].ant_count++], (const char *)s_content, MAX_ANT_LEN);
                                            }
                                            else
                                            {
                                                printf("Max ant reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"field") == 0)
                                        {
                                            if (e->senses[e->senses_count].field_count < MAX_FIELD)
                                            {
                                                strncpy(e->senses[e->senses_count].field[e->senses[e->senses_count].field_count++], (const char *)s_content, MAX_FIELD_LEN);
                                            }
                                            else
                                            {
                                                printf("Max field reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"misc") == 0)
                                        {
                                            if (e->senses[e->senses_count].misc_count < MAX_MISC)
                                            {
                                                strncpy(e->senses[e->senses_count].misc[e->senses[e->senses_count].misc_count++], (const char *)s_content, MAX_MISC_LEN);
                                            }
                                            else
                                            {
                                                printf("Max misc reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"s_inf") == 0)
                                        {
                                            if (e->senses[e->senses_count].s_inf_count < MAX_S_INF)
                                            {
                                                strncpy(e->senses[e->senses_count].s_inf[e->senses[e->senses_count].s_inf_count++], (const char *)s_content, MAX_S_INF_LEN);
                                            }
                                            else
                                            {
                                                printf("Max s_inf reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"lsource") == 0)
                                        {
                                            if (e->senses[e->senses_count].lsource_count < MAX_LSOURCE)
                                            {
                                                strncpy(e->senses[e->senses_count].lsource[e->senses[e->senses_count].lsource_count++], (const char *)s_content, MAX_LSOURCE_LEN);
                                            }
                                            else
                                            {
                                                printf("Max lsource reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"dial") == 0)
                                        {
                                            if (e->senses[e->senses_count].dial_count < MAX_DIAL)
                                            {
                                                strncpy(e->senses[e->senses_count].dial[e->senses[e->senses_count].dial_count++], (const char *)s_content, MAX_DIAL_LEN);
                                            }
                                            else
                                            {
                                                printf("Max dial reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"gloss") == 0)
                                        {
                                            // Scan for xml:lang attribute and set sense language if first gloss
                                            if (e->senses[e->senses_count].gloss_count == 0)
                                            {
                                                xmlChar *lang_attr = xmlGetProp(s_child, (const xmlChar *)"lang");
                                                if (lang_attr)
                                                {
                                                    if (xmlStrcmp(lang_attr, (const xmlChar *)"eng") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_EN;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fra") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_FR;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fre") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_FR;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"deu") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_DE;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ger") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_DE;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"rus") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_RU;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"spa") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ES;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"por") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_PT;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ita") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_IT;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"nld") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_NL;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"dut") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_NL;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"hun") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_HU;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"swe") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_SV;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ces") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_CS;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"cze") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_CS;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"pol") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_PL;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ron") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_RO;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"rum") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_RO;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"heb") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_HE;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ara") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_AR;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"tur") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_TR;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"tha") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_TH;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"vie") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_VI;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ind") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ID;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"msa") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_MS;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"kor") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_KO;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"zho") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ZH;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"cmn") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ZH_CN;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"yue") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ZH_TW;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"wuu") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ZH;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"nan") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ZH;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"hak") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_ZH;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fas") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_FA;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"per") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_FA;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"epo") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_EO;
                                                    }
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"slv") == 0)
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_SLV;
                                                    }
                                                    else
                                                    {
                                                        e->senses[e->senses_count].lang = KOTOBA_LANG_UNK;
                                                        printf("Unknown gloss language: %s\n", lang_attr);
                                                        printf ("For entry ent_seq: %d\n", e->ent_seq);
                                                    }
                                                    xmlFree(lang_attr);
                                                }
                                            }
                                            if (e->senses[e->senses_count].gloss_count < MAX_GLOSS)
                                            {
                                                strncpy(e->senses[e->senses_count].gloss[e->senses[e->senses_count].gloss_count++], (const char *)s_content, MAX_GLOSS_LEN);
                                            }
                                            else
                                            {
                                                printf("Max gloss reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"example") == 0)
                                        {
                                            if (e->senses[e->senses_count].examples_count < MAX_EXAMPLES)
                                            {
                                                xmlNodePtr e_child = NULL;
                                                for (e_child = s_child->children; e_child; e_child = e_child->next)
                                                {
                                                    if (e_child->type == XML_ELEMENT_NODE)
                                                    {
                                                        xmlChar *e_content = xmlNodeGetContent(e_child);
                                                        if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_srce") == 0)
                                                        {
                                                            strncpy(e->senses[e->senses_count].examples[e->senses[e->senses_count].examples_count].ex_srce, (const char *)e_content, MAX_EX_SRCE_LEN);
                                                        }
                                                        else if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_text") == 0)
                                                        {
                                                            strncpy(e->senses[e->senses_count].examples[e->senses[e->senses_count].examples_count].ex_text, (const char *)e_content, MAX_EX_TEXT_LEN);
                                                        }
                                                        else if (xmlStrcmp(e_child->name, (const xmlChar *)"ex_sent") == 0)
                                                        {
                                                            if (e->senses[e->senses_count].examples[e->senses[e->senses_count].examples_count].ex_sent_count < MAX_EX_SENT)
                                                            {
                                                                strncpy(e->senses[e->senses_count].examples[e->senses[e->senses_count].examples_count].ex_sent[e->senses[e->senses_count].examples[e->senses[e->senses_count].examples_count].ex_sent_count++], (const char *)e_content, MAX_EX_SENT_LEN);
                                                            }
                                                            else
                                                            {
                                                                printf("Max ex_sent reached for ent_seq: %d\n", e->ent_seq);
                                                            }
                                                        }
                                                        xmlFree(e_content);
                                                    }
                                                }
                                                e->senses[e->senses_count].examples_count++;
                                            }
                                            else
                                            {
                                                printf("Max examples reached for ent_seq: %d\n", e->ent_seq);
                                            }
                                        }
                                        xmlFree(s_content);
                                    }
                                }
                                e->senses_count++;
                            }
                            else 
                            {
                                printf("Max senses reached for ent_seq: %d\n", e->ent_seq);
                            }
                        }
                        xmlFree(content);
                    }
                }
                write_entry_with_index(output_filename, idx_filename, e);
                (*entry_count)++;
            }
        }
    }
    free(e);
}
      

void create_kotoba_bin()
{

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

    FILE *output_filename = fopen(dict_path, "wb");
    FILE *idx_filename = fopen(idx_path, "wb");

    kotoba_write_bin_header(output_filename, 0); // placeholder
    kotoba_write_idx_header(idx_filename, 0);    // placeholder
    parse_jmdict(root, output_filename, idx_filename, &entry_count);
    printf("Parsed %llu entries.\n", entry_count);
    // Rewrite headers with correct entry count
    kotoba_write_bin_header(output_filename, (uint32_t)entry_count);
    kotoba_write_idx_header(idx_filename, (uint32_t)entry_count);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

int main()
{
#if _WIN32
    system("chcp 65001"); // Change code page to UTF-8 for Windows
#endif

    // writing stuff
    create_kotoba_bin();

    // loading stuff
    /*
    kotoba_loader loader;

    if (kotoba_loader_open(&loader, dict_path, idx_path))
    {
        printf("Entries: %u\n", loader.bin_hdr->entry_count);

        entry_view ev;
        if (kotoba_loader_get(&loader, 1, &ev))
        {
            printf("First ent_seq: %u\n", ev.eb->ent_seq);
        }

        kotoba_loader_close(&loader);
    }
    */

    return 0;
}