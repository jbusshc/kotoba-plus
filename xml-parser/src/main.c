#include <stdio.h>
#include <stdlib.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <stdbool.h>
#if _WIN32
#include <windows.h>
#endif

#include "../../kotoba-core/include/kotoba_types.h"
//#include "../../kotoba-core/include/kotoba_writer.h"
//#include "../../kotoba-core/include/kotoba_loader.h"
//#include "../../kotoba-core/include/kotoba_dict.h"

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
    entry e;
    for (xmlNodePtr cur_node = root->children; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (xmlStrcmp(cur_node->name, (const xmlChar *)"entry") == 0)
            {
                memset(&e, 0, sizeof(entry));
                for (xmlNodePtr child_node = cur_node->children; child_node; child_node = child_node->next)
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
                            if (e.k_elements_count < MAX_K_ELEMENTS)
                            {
                                k_ele *k = &e.k_elements[e.k_elements_count];
                                memset(k, 0, sizeof(k_ele));
                                for (xmlNodePtr k_child = child_node->children; k_child; k_child = k_child->next)
                                {
                                    if (k_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *k_content = xmlNodeGetContent(k_child);
                                        if (xmlStrcmp(k_child->name, (const xmlChar *)"keb") == 0)
                                        {
                                            strncpy(k->keb, (const char *)k_content, MAX_KEB_LEN);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_inf") == 0)
                                        {
                                            if (k->ke_inf_count < MAX_KE_INF)
                                                strncpy(k->ke_inf[k->ke_inf_count++], (const char *)k_content, MAX_KE_INF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_KE_INF (%d)\n", e.ent_seq, MAX_KE_INF);
                                        }
                                        else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_pri") == 0)
                                        {
                                            if (k->ke_pri_count < MAX_KE_PRI)
                                                strncpy(k->ke_pri[k->ke_pri_count++], (const char *)k_content, MAX_KE_PRI_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_KE_PRI (%d)\n", e.ent_seq, MAX_KE_PRI);
                                        }
                                        xmlFree(k_content);
                                    }
                                }
                                e.k_elements_count++;
                            }
                            else
                            {
                                printf("[WARN] ent_seq=%u: se excedió MAX_K_ELEMENTS (%d)\n", e.ent_seq, MAX_K_ELEMENTS);
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"r_ele") == 0)
                        {
                            if (e.r_elements_count < MAX_R_ELEMENTS)
                            {
                                r_ele *r = &e.r_elements[e.r_elements_count];
                                memset(r, 0, sizeof(r_ele));
                                for (xmlNodePtr r_child = child_node->children; r_child; r_child = r_child->next)
                                {
                                    if (r_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *r_content = xmlNodeGetContent(r_child);
                                        if (xmlStrcmp(r_child->name, (const xmlChar *)"reb") == 0)
                                        {
                                            strncpy(r->reb, (const char *)r_content, MAX_REB_LEN);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_nokanji") == 0)
                                        {
                                            r->re_nokanji = true;
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_restr") == 0)
                                        {
                                            if (r->re_restr_count < MAX_RE_RESTR)
                                                strncpy(r->re_restr[r->re_restr_count++], (const char *)r_content, MAX_RE_RESTR_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_RE_RESTR (%d)\n", e.ent_seq, MAX_RE_RESTR);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_inf") == 0)
                                        {
                                            if (r->re_inf_count < MAX_RE_INF)
                                                strncpy(r->re_inf[r->re_inf_count++], (const char *)r_content, MAX_RE_INF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_RE_INF (%d)\n", e.ent_seq, MAX_RE_INF);
                                        }
                                        else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_pri") == 0)
                                        {
                                            if (r->re_pri_count < MAX_RE_PRI)
                                                strncpy(r->re_pri[r->re_pri_count++], (const char *)r_content, MAX_RE_PRI_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_RE_PRI (%d)\n", e.ent_seq, MAX_RE_PRI);
                                        }
                                        xmlFree(r_content);
                                    }
                                }
                                e.r_elements_count++;
                            }
                            else
                            {
                                printf("[WARN] ent_seq=%u: se excedió MAX_R_ELEMENTS (%d)\n", e.ent_seq, MAX_R_ELEMENTS);
                            }
                        }
                        else if (xmlStrcmp(child_node->name, (const xmlChar *)"sense") == 0)
                        {
                            if (e.senses_count < MAX_SENSES)
                            {
                                sense *s = &e.senses[e.senses_count];
                                memset(s, 0, sizeof(sense));
                                s->lang = KOTOBA_LANG_EN; // default
                                for (xmlNodePtr s_child = child_node->children; s_child; s_child = s_child->next)
                                {
                                    if (s_child->type == XML_ELEMENT_NODE)
                                    {
                                        xmlChar *s_content = xmlNodeGetContent(s_child);
                                        if (xmlStrcmp(s_child->name, (const xmlChar *)"stagk") == 0)
                                        {
                                            if (s->stagk_count < MAX_STAGK)
                                                strncpy(s->stagk[s->stagk_count++], (const char *)s_content, MAX_STAGK_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_STAGK (%d)\n", e.ent_seq, MAX_STAGK);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"stagr") == 0)
                                        {
                                            if (s->stagr_count < MAX_STAGR)
                                                strncpy(s->stagr[s->stagr_count++], (const char *)s_content, MAX_STAGR_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_STAGR (%d)\n", e.ent_seq, MAX_STAGR);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"pos") == 0)
                                        {
                                            if (s->pos_count < MAX_POS)
                                                strncpy(s->pos[s->pos_count++], (const char *)s_content, MAX_POS_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_POS (%d)\n", e.ent_seq, MAX_POS);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"xref") == 0)
                                        {
                                            if (s->xref_count < MAX_XREF)
                                                strncpy(s->xref[s->xref_count++], (const char *)s_content, MAX_XREF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_XREF (%d)\n", e.ent_seq, MAX_XREF);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"ant") == 0)
                                        {
                                            if (s->ant_count < MAX_ANT)
                                                strncpy(s->ant[s->ant_count++], (const char *)s_content, MAX_ANT_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_ANT (%d)\n", e.ent_seq, MAX_ANT);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"field") == 0)
                                        {
                                            if (s->field_count < MAX_FIELD)
                                                strncpy(s->field[s->field_count++], (const char *)s_content, MAX_FIELD_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_FIELD (%d)\n", e.ent_seq, MAX_FIELD);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"misc") == 0)
                                        {
                                            if (s->misc_count < MAX_MISC)
                                                strncpy(s->misc[s->misc_count++], (const char *)s_content, MAX_MISC_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_MISC (%d)\n", e.ent_seq, MAX_MISC);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"s_inf") == 0)
                                        {
                                            if (s->s_inf_count < MAX_S_INF)
                                                strncpy(s->s_inf[s->s_inf_count++], (const char *)s_content, MAX_S_INF_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_S_INF (%d)\n", e.ent_seq, MAX_S_INF);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"lsource") == 0)
                                        {
                                            if (s->lsource_count < MAX_LSOURCE)
                                                strncpy(s->lsource[s->lsource_count++], (const char *)s_content, MAX_LSOURCE_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_LSOURCE (%d)\n", e.ent_seq, MAX_LSOURCE);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"dial") == 0)
                                        {
                                            if (s->dial_count < MAX_DIAL)
                                                strncpy(s->dial[s->dial_count++], (const char *)s_content, MAX_DIAL_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_DIAL (%d)\n", e.ent_seq, MAX_DIAL);
                                        }
                                        else if (xmlStrcmp(s_child->name, (const xmlChar *)"gloss") == 0)
                                        {
                                            // Set lang if first gloss and lang attribute exists
                                            if (s->gloss_count == 0)
                                            {
                                                xmlChar *lang_attr = xmlGetProp(s_child, (const xmlChar *)"lang");
                                                if (lang_attr)
                                                {
                                                    if (xmlStrcmp(lang_attr, (const xmlChar *)"eng") == 0)
                                                        s->lang = KOTOBA_LANG_EN;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fra") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"fre") == 0)
                                                        s->lang = KOTOBA_LANG_FR;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"deu") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"ger") == 0)
                                                        s->lang = KOTOBA_LANG_DE;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"rus") == 0)
                                                        s->lang = KOTOBA_LANG_RU;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"spa") == 0)
                                                        s->lang = KOTOBA_LANG_ES;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"por") == 0)
                                                        s->lang = KOTOBA_LANG_PT;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ita") == 0)
                                                        s->lang = KOTOBA_LANG_IT;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"nld") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"dut") == 0)
                                                        s->lang = KOTOBA_LANG_NL;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"hun") == 0)
                                                        s->lang = KOTOBA_LANG_HU;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"swe") == 0)
                                                        s->lang = KOTOBA_LANG_SV;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ces") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"cze") == 0)
                                                        s->lang = KOTOBA_LANG_CS;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"pol") == 0)
                                                        s->lang = KOTOBA_LANG_PL;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ron") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"rum") == 0)
                                                        s->lang = KOTOBA_LANG_RO;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"heb") == 0)
                                                        s->lang = KOTOBA_LANG_HE;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ara") == 0)
                                                        s->lang = KOTOBA_LANG_AR;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"tur") == 0)
                                                        s->lang = KOTOBA_LANG_TR;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"tha") == 0)
                                                        s->lang = KOTOBA_LANG_TH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"vie") == 0)
                                                        s->lang = KOTOBA_LANG_VI;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"ind") == 0)
                                                        s->lang = KOTOBA_LANG_ID;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"msa") == 0)
                                                        s->lang = KOTOBA_LANG_MS;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"kor") == 0)
                                                        s->lang = KOTOBA_LANG_KO;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"zho") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"cmn") == 0)
                                                        s->lang = KOTOBA_LANG_ZH_CN;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"yue") == 0)
                                                        s->lang = KOTOBA_LANG_ZH_TW;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"wuu") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"nan") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"hak") == 0)
                                                        s->lang = KOTOBA_LANG_ZH;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"fas") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"per") == 0)
                                                        s->lang = KOTOBA_LANG_FA;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"epo") == 0)
                                                        s->lang = KOTOBA_LANG_EO;
                                                    else if (xmlStrcmp(lang_attr, (const xmlChar *)"slv") == 0)
                                                        s->lang = KOTOBA_LANG_SLV;
                                                    else
                                                        s->lang = KOTOBA_LANG_UNK;
                                                    xmlFree(lang_attr);
                                                }
                                            }
                                            if (s->gloss_count < MAX_GLOSS)
                                                strncpy(s->gloss[s->gloss_count++], (const char *)s_content, MAX_GLOSS_LEN);
                                            else
                                                printf("[WARN] ent_seq=%u: se excedió MAX_GLOSS (%d)\n", e.ent_seq, MAX_GLOSS);
                                        }
                                        xmlFree(s_content);
                                    }
                                }
                                e.senses_count++;
                            }
                            else
                            {
                                printf("[WARN] ent_seq=%u: se excedió MAX_SENSES (%d)\n", e.ent_seq, MAX_SENSES);
                            }
                        }
                        xmlFree(content);
                    }
                }
                //write_entry_with_index(output_filename, idx_filename, &e);
                (*entry_count)++;
            }
        }
    }
}


// Parses a single <entry> XML node into an entry struct
static void parse_entry_from_xml(xmlNodePtr entry_node, entry *e)
{
    // Zero out the entry struct
    memset(e, 0, sizeof(entry));

    for (xmlNodePtr child = entry_node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;

        xmlChar *content = xmlNodeGetContent(child);

        if (xmlStrcmp(child->name, (const xmlChar *)"ent_seq") == 0)
        {
            e->ent_seq = atoi((const char *)content);
        }
        else if (xmlStrcmp(child->name, (const xmlChar *)"k_ele") == 0)
        {
            if (e->k_elements_count < MAX_K_ELEMENTS)
            {
                k_ele *k = &e->k_elements[e->k_elements_count];
                memset(k, 0, sizeof(k_ele));
                for (xmlNodePtr k_child = child->children; k_child; k_child = k_child->next)
                {
                    if (k_child->type != XML_ELEMENT_NODE)
                        continue;
                    xmlChar *k_content = xmlNodeGetContent(k_child);
                    if (xmlStrcmp(k_child->name, (const xmlChar *)"keb") == 0)
                    {
                        strncpy(k->keb, (const char *)k_content, MAX_KEB_LEN);
                    }
                    else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_inf") == 0)
                    {
                        if (k->ke_inf_count < MAX_KE_INF)
                            strncpy(k->ke_inf[k->ke_inf_count++], (const char *)k_content, MAX_KE_INF_LEN);
                    }
                    else if (xmlStrcmp(k_child->name, (const xmlChar *)"ke_pri") == 0)
                    {
                        if (k->ke_pri_count < MAX_KE_PRI)
                            strncpy(k->ke_pri[k->ke_pri_count++], (const char *)k_content, MAX_KE_PRI_LEN);
                    }
                    xmlFree(k_content);
                }
                e->k_elements_count++;
            }
        }
        else if (xmlStrcmp(child->name, (const xmlChar *)"r_ele") == 0)
        {
            if (e->r_elements_count < MAX_R_ELEMENTS)
            {
                r_ele *r = &e->r_elements[e->r_elements_count];
                memset(r, 0, sizeof(r_ele));
                for (xmlNodePtr r_child = child->children; r_child; r_child = r_child->next)
                {
                    if (r_child->type != XML_ELEMENT_NODE)
                        continue;
                    xmlChar *r_content = xmlNodeGetContent(r_child);
                    if (xmlStrcmp(r_child->name, (const xmlChar *)"reb") == 0)
                    {
                        strncpy(r->reb, (const char *)r_content, MAX_REB_LEN);
                    }
                    else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_nokanji") == 0)
                    {
                        r->re_nokanji = true;
                    }
                    else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_restr") == 0)
                    {
                        if (r->re_restr_count < MAX_RE_RESTR)
                            strncpy(r->re_restr[r->re_restr_count++], (const char *)r_content, MAX_RE_RESTR_LEN);
                    }
                    else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_inf") == 0)
                    {
                        if (r->re_inf_count < MAX_RE_INF)
                            strncpy(r->re_inf[r->re_inf_count++], (const char *)r_content, MAX_RE_INF_LEN);
                    }
                    else if (xmlStrcmp(r_child->name, (const xmlChar *)"re_pri") == 0)
                    {
                        if (r->re_pri_count < MAX_RE_PRI)
                            strncpy(r->re_pri[r->re_pri_count++], (const char *)r_content, MAX_RE_PRI_LEN);
                    }
                    xmlFree(r_content);
                }
                e->r_elements_count++;
            }
        }
        else if (xmlStrcmp(child->name, (const xmlChar *)"sense") == 0)
        {
            if (e->senses_count < MAX_SENSES)
            {
                sense *s = &e->senses[e->senses_count];
                memset(s, 0, sizeof(sense));
                s->lang = KOTOBA_LANG_EN; // default
                for (xmlNodePtr s_child = child->children; s_child; s_child = s_child->next)
                {
                    if (s_child->type != XML_ELEMENT_NODE)
                        continue;
                    xmlChar *s_content = xmlNodeGetContent(s_child);
                    if (xmlStrcmp(s_child->name, (const xmlChar *)"stagk") == 0)
                    {
                        if (s->stagk_count < MAX_STAGK)
                            strncpy(s->stagk[s->stagk_count++], (const char *)s_content, MAX_STAGK_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"stagr") == 0)
                    {
                        if (s->stagr_count < MAX_STAGR)
                            strncpy(s->stagr[s->stagr_count++], (const char *)s_content, MAX_STAGR_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"pos") == 0)
                    {
                        if (s->pos_count < MAX_POS)
                            strncpy(s->pos[s->pos_count++], (const char *)s_content, MAX_POS_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"xref") == 0)
                    {
                        if (s->xref_count < MAX_XREF)
                            strncpy(s->xref[s->xref_count++], (const char *)s_content, MAX_XREF_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"ant") == 0)
                    {
                        if (s->ant_count < MAX_ANT)
                            strncpy(s->ant[s->ant_count++], (const char *)s_content, MAX_ANT_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"field") == 0)
                    {
                        if (s->field_count < MAX_FIELD)
                            strncpy(s->field[s->field_count++], (const char *)s_content, MAX_FIELD_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"misc") == 0)
                    {
                        if (s->misc_count < MAX_MISC)
                            strncpy(s->misc[s->misc_count++], (const char *)s_content, MAX_MISC_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"s_inf") == 0)
                    {
                        if (s->s_inf_count < MAX_S_INF)
                            strncpy(s->s_inf[s->s_inf_count++], (const char *)s_content, MAX_S_INF_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"lsource") == 0)
                    {
                        if (s->lsource_count < MAX_LSOURCE)
                            strncpy(s->lsource[s->lsource_count++], (const char *)s_content, MAX_LSOURCE_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"dial") == 0)
                    {
                        if (s->dial_count < MAX_DIAL)
                            strncpy(s->dial[s->dial_count++], (const char *)s_content, MAX_DIAL_LEN);
                    }
                    else if (xmlStrcmp(s_child->name, (const xmlChar *)"gloss") == 0)
                    {
                        // Set lang if first gloss and lang attribute exists
                        if (s->gloss_count == 0)
                        {
                            xmlChar *lang_attr = xmlGetProp(s_child, (const xmlChar *)"lang");
                            if (lang_attr)
                            {
                                if (xmlStrcmp(lang_attr, (const xmlChar *)"eng") == 0)
                                    s->lang = KOTOBA_LANG_EN;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"fra") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"fre") == 0)
                                    s->lang = KOTOBA_LANG_FR;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"deu") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"ger") == 0)
                                    s->lang = KOTOBA_LANG_DE;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"rus") == 0)
                                    s->lang = KOTOBA_LANG_RU;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"spa") == 0)
                                    s->lang = KOTOBA_LANG_ES;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"por") == 0)
                                    s->lang = KOTOBA_LANG_PT;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"ita") == 0)
                                    s->lang = KOTOBA_LANG_IT;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"nld") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"dut") == 0)
                                    s->lang = KOTOBA_LANG_NL;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"hun") == 0)
                                    s->lang = KOTOBA_LANG_HU;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"swe") == 0)
                                    s->lang = KOTOBA_LANG_SV;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"ces") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"cze") == 0)
                                    s->lang = KOTOBA_LANG_CS;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"pol") == 0)
                                    s->lang = KOTOBA_LANG_PL;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"ron") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"rum") == 0)
                                    s->lang = KOTOBA_LANG_RO;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"heb") == 0)
                                    s->lang = KOTOBA_LANG_HE;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"ara") == 0)
                                    s->lang = KOTOBA_LANG_AR;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"tur") == 0)
                                    s->lang = KOTOBA_LANG_TR;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"tha") == 0)
                                    s->lang = KOTOBA_LANG_TH;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"vie") == 0)
                                    s->lang = KOTOBA_LANG_VI;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"ind") == 0)
                                    s->lang = KOTOBA_LANG_ID;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"msa") == 0)
                                    s->lang = KOTOBA_LANG_MS;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"kor") == 0)
                                    s->lang = KOTOBA_LANG_KO;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"zho") == 0)
                                    s->lang = KOTOBA_LANG_ZH;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"cmn") == 0)
                                    s->lang = KOTOBA_LANG_ZH_CN;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"yue") == 0)
                                    s->lang = KOTOBA_LANG_ZH_TW;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"wuu") == 0)
                                    s->lang = KOTOBA_LANG_ZH;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"nan") == 0)
                                    s->lang = KOTOBA_LANG_ZH;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"hak") == 0)
                                    s->lang = KOTOBA_LANG_ZH;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"fas") == 0 || xmlStrcmp(lang_attr, (const xmlChar *)"per") == 0)
                                    s->lang = KOTOBA_LANG_FA;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"epo") == 0)
                                    s->lang = KOTOBA_LANG_EO;
                                else if (xmlStrcmp(lang_attr, (const xmlChar *)"slv") == 0)
                                    s->lang = KOTOBA_LANG_SLV;
                                else
                                    s->lang = KOTOBA_LANG_UNK;
                                xmlFree(lang_attr);
                            }
                        }
                        if (s->gloss_count < MAX_GLOSS)
                            strncpy(s->gloss[s->gloss_count++], (const char *)s_content, MAX_GLOSS_LEN);
                    }
                    xmlFree(s_content);
                }
                e->senses_count++;
            }
        }

        xmlFree(content);
    }
}





int main()
{
#if _WIN32
    system("chcp 65001"); // Change code page to UTF-8 for Windows
#endif

    // writing stuff
     //create_kotoba_bin();

    // validating stuff

    // validate();

    // loading stuff
    
    printf("sizeof(kotoba_bin_header) = %zu bytes\n", sizeof(kotoba_bin_header));
    printf("sizeof(kotoba_idx_header) = %zu bytes\n", sizeof(kotoba_idx_header));
    printf("sizeof(entry_bin) = %zu bytes\n", sizeof(entry_bin));
    printf("sizeof(k_ele_bin) = %zu bytes\n", sizeof(k_ele_bin));
    printf("sizeof(r_ele_bin) = %zu bytes\n", sizeof(r_ele_bin));
    printf("sizeof(sense_bin) = %zu bytes\n", sizeof(sense_bin));



    return 0;
}