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
#include "../../kotoba-core/include/kotoba_viewer.h"

const char *dict_path = "dict.kotoba";
const char *idx_path = "dict.kotoba.idx";

#define ROUTE_JMDICT_IDX 0
#define MAX_ENTRIES 209000
char *routes[] = {
    "../assets/JMdict",
};
int routes_count = sizeof(routes) / sizeof(routes[0]);
void parse_jmdict(xmlNodePtr root, kotoba_writer *writer)
{
    writer->entry_count = 0;
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
                // write_entry_with_index(output_filename, idx_filename, &e);
                kotoba_writer_write_entry(writer, &e);
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

void print_entry(const kotoba_dict *d, uint32_t i)
{
    const entry_bin *e = kotoba_entry(d, i);
    if (!e)
        return;

    printf("Entry %u\n", i);
    printf("  ent_seq = %d\n", e->ent_seq);
    /* -------------------------
        * Kanji
        * ------------------------- */

    printf("  k_elements_count = %u\n", e->k_elements_count);

    for (uint32_t i = 0; i < e->k_elements_count; ++i)
    {
        const k_ele_bin *k = kotoba_k_ele(d, e, i);
        kotoba_str keb = kotoba_keb(d, k);

        printf("  kanji[%u]: %.*s\n",
                i, keb.len, keb.ptr);
    }

    printf("  r_elements_count = %u\n", e->r_elements_count);
    /* -------------------------
        * Readings
        * ------------------------- */
    for (uint32_t i = 0; i < e->r_elements_count; ++i)
    {
        const r_ele_bin *r = kotoba_r_ele(d, e, i);
        printf("  post kotoba_r_ele\n");
        kotoba_str reb = kotoba_reb(d, r);

        printf("  reading[%u]: %.*s\n",
                i, reb.len, reb.ptr);
    }

    printf("  senses_count = %u\n", e->senses_count);
    /* -------------------------
        * Senses
        * ------------------------- */
    for (uint32_t si = 0; si < e->senses_count; ++si)
    {
        const sense_bin *s = kotoba_sense(d, e, si);

        printf("  sense[%u] (lang=%u)\n", si, s->lang);

        /* POS */
        for (uint32_t i = 0; i < s->pos_count; ++i)
        {
            kotoba_str pos = kotoba_pos(d, s, i);
            printf("    pos: %.*s\n", pos.len, pos.ptr);
        }

        /* Gloss */
        for (uint32_t i = 0; i < s->gloss_count; ++i)
        {
            kotoba_str g = kotoba_gloss(d, s, i);
            printf("    gloss: %.*s\n", g.len, g.ptr);
        }

        /* Misc (ejemplo) */
        for (uint32_t i = 0; i < s->misc_count; ++i)
        {
            kotoba_str m = kotoba_misc(d, s, i);
            printf("    misc: %.*s\n", m.len, m.ptr);
        }

        for (uint32_t i = 0; i < s->stagk_count; ++i)
        {
            kotoba_str stg = kotoba_stagk(d, s, i);
            printf("    stagk: %.*s\n", stg.len, stg.ptr);
        }

        for (uint32_t i = 0; i < s->stagr_count; ++i)
        {
            kotoba_str stg = kotoba_stagr(d, s, i);
            printf("    stagr: %.*s\n", stg.len, stg.ptr);
        }

        for (uint32_t i = 0; i < s->xref_count; ++i)
        {
            kotoba_str xref = kotoba_xref(d, s, i);
            printf("    xref: %.*s\n", xref.len, xref.ptr);
        }

        for (uint32_t i = 0; i < s->ant_count; ++i)
        {
            kotoba_str ant = kotoba_ant(d, s, i);
            printf("    ant: %.*s\n", ant.len, ant.ptr);
        }

        for (uint32_t i = 0; i < s->field_count; ++i)
        {
            kotoba_str field = kotoba_field(d, s, i);
            printf("    field: %.*s\n", field.len, field.ptr);
        }

        for (uint32_t i = 0; i < s->s_inf_count; ++i)
        {
            kotoba_str s_inf = kotoba_s_inf(d, s, i);
            printf("    s_inf: %.*s\n", s_inf.len, s_inf.ptr);
        }

        for (uint32_t i = 0; i < s->lsource_count; ++i)
        {
            kotoba_str lsource = kotoba_lsource(d, s, i);
            printf("    lsource: %.*s\n", lsource.len, lsource.ptr);
        }

        for (uint32_t i = 0; i < s->dial_count; ++i)
        {
            kotoba_str dial = kotoba_dial(d, s, i);
            printf("    dial: %.*s\n", dial.len, dial.ptr);
        }
    }
}



int main()
{
#if _WIN32
    system("chcp 65001"); // Change code page to UTF-8 for Windows
#endif

    /*
    // xml parsing stuff
    xmlDocPtr doc = xmlReadFile(routes[0], NULL, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    if (doc == NULL)
    {
        fprintf(stderr, "Failed to parse XML file: %s\n", routes[0]);
        return -1;
    }
    xmlNodePtr root = xmlDocGetRootElement(doc);
    // writing stuff
    kotoba_writer writer;
    kotoba_writer_open(&writer, "dict.kotoba", "dict.kotoba.idx");
    parse_jmdict(root, &writer);
    kotoba_writer_close(&writer);
    printf("Total entries written: %u\n", writer.entry_count);


    */

    ///*
    // loading stuff
    kotoba_dict dict;
    if (!kotoba_dict_open(&dict, "dict.kotoba", "dict.kotoba.idx"))
    {
        fprintf(stderr, "Failed to open dictionary files.\n");
    }
    printf("Total entries in dictionary: %u\n", dict.entry_count);


    const entry_bin *ebin;
    const int test_id = 99;
    ebin = malloc(sizeof(entry_bin));
    if ((ebin = kotoba_dict_get_entry(&dict, test_id)) != NULL)
    {
        printf("Entry 99: ent_seq=%d, k_elements_count=%u, r_elements_count=%u, senses_count=%u\n",
               ebin->ent_seq,
               ebin->k_elements_count,
               ebin->r_elements_count,
               ebin->senses_count);
        print_entry(&dict, test_id);
    }
    else
    {
        printf("Failed to get entry 100.\n");
    }


    kotoba_dict_close(&dict);

    //*/


    /*
    xmlFreeDoc(doc);
    xmlCleanupParser();
    */
    return 0;
}