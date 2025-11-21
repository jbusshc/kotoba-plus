#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libtango.h"

#ifdef _WIN32
#include <windows.h>
// Configura la consola para UTF-8 en Windows
void set_console_utf8() {
    system("chcp 65001 > nul");
}
#endif


void imprimir_resultado(const TangoSearchResult* result, void* userdata) {
    if (!result) return;

    printf("🆔 Entrada %d\n", result->ent_seq);
    printf("   Kanji: %s\n", result->kanjis && *result->kanjis ? result->kanjis : "(sin kanji)");
    printf("   Lectura: %s\n", result->readings);
    printf("   Glosas: %s\n", result->glosses);
    printf("   Prioridad: %d\n", result->priority);
    printf("\n");
}

void imprimir_entry(const entry* e, void* userdata) {
    if (!e) return;

    printf("🆔 Entrada %d\n", e->ent_seq);
    printf("   Prioridad: %d\n", e->priority);

    // Kanji elements
    printf("   Kanji(s): ");
    if (e->k_elements_count == 0) {
        printf("(sin kanji)\n");
    } else {
        for (int i = 0; i < e->k_elements_count; i++) {
            printf("%s", e->k_elements[i].keb);
            if (i < e->k_elements_count - 1) printf(", ");
        }
        printf("\n");
    }

    // Reading elements
    printf("   Lectura(s): ");
    if (e->r_elements_count == 0) {
        printf("(sin lectura)\n");
    } else {
        for (int i = 0; i < e->r_elements_count; i++) {
            printf("%s", e->r_elements[i].reb);
            if (i < e->r_elements_count - 1) printf(", ");
        }
        printf("\n");
    }

    // Senses
    for (int i = 0; i < e->senses_count; i++) {
        printf("   Sentido %d:\n", i + 1);
        printf("      Glosas: ");
        for (int j = 0; j < e->senses[i].gloss_count; j++) {
            printf("%s", e->senses[i].gloss[j]);
            if (j < e->senses[i].gloss_count - 1) printf("; ");
        }
        printf("\n");
        if (e->senses[i].pos_count > 0) {
            printf("      POS: ");
            for (int j = 0; j < e->senses[i].pos_count; j++) {
                printf("%s", e->senses[i].pos[j]);
                if (j < e->senses[i].pos_count - 1) printf(", ");
            }
            printf("\n");
        }
        if (e->senses[i].misc_count > 0) {
            printf("      Misc: ");
            for (int j = 0; j < e->senses[i].misc_count; j++) {
                printf("%s", e->senses[i].misc[j]);
                if (j < e->senses[i].misc_count - 1) printf(", ");
            }
            printf("\n");
        }
    }
    printf("\n");
}

#include <cjson/cJSON.h>

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

void parse_entry_json(const char* json, entry* e) {
    cJSON* root = cJSON_Parse(json);
    if (!root) return;

    // k_ele
    cJSON* k_ele = cJSON_GetObjectItem(root, "k_ele");
    if (k_ele && cJSON_IsArray(k_ele)) {
        e->k_elements_count = cJSON_GetArraySize(k_ele);
        for (int i = 0; i < e->k_elements_count; i++) {
            cJSON* kobj = cJSON_GetArrayItem(k_ele, i);
            cJSON* keb = cJSON_GetObjectItem(kobj, "keb");
            if (keb && cJSON_IsString(keb)) {
                strncpy(e->k_elements[i].keb, keb->valuestring, sizeof(e->k_elements[i].keb)-1);
            }
            cJSON* ke_inf = cJSON_GetObjectItem(kobj, "ke_inf");
            if (ke_inf && cJSON_IsArray(ke_inf)) {
                e->k_elements[i].ke_inf_count = cJSON_GetArraySize(ke_inf);
                for (int j = 0; j < e->k_elements[i].ke_inf_count; j++) {
                    cJSON* inf = cJSON_GetArrayItem(ke_inf, j);
                    if (inf && cJSON_IsString(inf)) {
                        strncpy(e->k_elements[i].ke_inf[j], inf->valuestring, sizeof(e->k_elements[i].ke_inf[j])-1);
                    }
                }
            }
            cJSON* ke_pri = cJSON_GetObjectItem(kobj, "ke_pri");
            if (ke_pri && cJSON_IsArray(ke_pri)) {
                e->k_elements[i].ke_pri_count = cJSON_GetArraySize(ke_pri);
                for (int j = 0; j < e->k_elements[i].ke_pri_count; j++) {
                    cJSON* pri = cJSON_GetArrayItem(ke_pri, j);
                    if (pri && cJSON_IsString(pri)) {
                        strncpy(e->k_elements[i].ke_pri[j], pri->valuestring, sizeof(e->k_elements[i].ke_pri[j])-1);
                    }
                }
            }
        }
    }

    // r_ele
    cJSON* r_ele = cJSON_GetObjectItem(root, "r_ele");
    if (r_ele && cJSON_IsArray(r_ele)) {
        e->r_elements_count = cJSON_GetArraySize(r_ele);
        for (int i = 0; i < e->r_elements_count; i++) {
            cJSON* robj = cJSON_GetArrayItem(r_ele, i);
            cJSON* reb = cJSON_GetObjectItem(robj, "reb");
            if (reb && cJSON_IsString(reb)) {
                strncpy(e->r_elements[i].reb, reb->valuestring, sizeof(e->r_elements[i].reb)-1);
            }
            cJSON* re_inf = cJSON_GetObjectItem(robj, "re_inf");
            if (re_inf && cJSON_IsArray(re_inf)) {
                e->r_elements[i].re_inf_count = cJSON_GetArraySize(re_inf);
                for (int j = 0; j < e->r_elements[i].re_inf_count; j++) {
                    cJSON* inf = cJSON_GetArrayItem(re_inf, j);
                    if (inf && cJSON_IsString(inf)) {
                        strncpy(e->r_elements[i].re_inf[j], inf->valuestring, sizeof(e->r_elements[i].re_inf[j])-1);
                    }
                }
            }
            cJSON* re_pri = cJSON_GetObjectItem(robj, "re_pri");
            if (re_pri && cJSON_IsArray(re_pri)) {
                e->r_elements[i].re_pri_count = cJSON_GetArraySize(re_pri);
                for (int j = 0; j < e->r_elements[i].re_pri_count; j++) {
                    cJSON* pri = cJSON_GetArrayItem(re_pri, j);
                    if (pri && cJSON_IsString(pri)) {
                        strncpy(e->r_elements[i].re_pri[j], pri->valuestring, sizeof(e->r_elements[i].re_pri[j])-1);
                    }
                }
            }
        }
    }

    // sense
    cJSON* sense = cJSON_GetObjectItem(root, "sense");
    if (sense && cJSON_IsArray(sense)) {
        e->senses_count = cJSON_GetArraySize(sense);
        for (int i = 0; i < e->senses_count; i++) {
            cJSON* sobj = cJSON_GetArrayItem(sense, i);

            // pos
            cJSON* pos = cJSON_GetObjectItem(sobj, "pos");
            if (pos && cJSON_IsArray(pos)) {
                e->senses[i].pos_count = cJSON_GetArraySize(pos);
                for (int j = 0; j < e->senses[i].pos_count; j++) {
                    cJSON* p = cJSON_GetArrayItem(pos, j);
                    if (p && cJSON_IsString(p)) {
                        strncpy(e->senses[i].pos[j], p->valuestring, sizeof(e->senses[i].pos[j])-1);
                    }
                }
            }
            // misc
            cJSON* misc = cJSON_GetObjectItem(sobj, "misc");
            if (misc && cJSON_IsArray(misc)) {
                e->senses[i].misc_count = cJSON_GetArraySize(misc);
                for (int j = 0; j < e->senses[i].misc_count; j++) {
                    cJSON* m = cJSON_GetArrayItem(misc, j);
                    if (m && cJSON_IsString(m)) {
                        strncpy(e->senses[i].misc[j], m->valuestring, sizeof(e->senses[i].misc[j])-1);
                    }
                }
            }
            // gloss
            cJSON* gloss = cJSON_GetObjectItem(sobj, "gloss");
            if (gloss && cJSON_IsArray(gloss)) {
                e->senses[i].gloss_count = cJSON_GetArraySize(gloss);
                for (int j = 0; j < e->senses[i].gloss_count; j++) {
                    cJSON* g = cJSON_GetArrayItem(gloss, j);
                    if (g && cJSON_IsString(g)) {
                        strncpy(e->senses[i].gloss[j], g->valuestring, sizeof(e->senses[i].gloss[j])-1);
                    }
                }
            }
            // example
            cJSON* example = cJSON_GetObjectItem(sobj, "example");
            if (example && cJSON_IsObject(example)) {
                cJSON* ex_srce = cJSON_GetObjectItem(example, "ex_srce");
                if (ex_srce && cJSON_IsString(ex_srce)) {
                    strncpy(e->senses[i].examples[0].ex_srce, ex_srce->valuestring, sizeof(e->senses[i].examples[0].ex_srce)-1);
                }
                cJSON* ex_text = cJSON_GetObjectItem(example, "ex_text");
                if (ex_text && cJSON_IsString(ex_text)) {
                    strncpy(e->senses[i].examples[0].ex_text, ex_text->valuestring, sizeof(e->senses[i].examples[0].ex_text)-1);
                }
                cJSON* ex_sent = cJSON_GetObjectItem(example, "ex_sent");
                if (ex_sent && cJSON_IsArray(ex_sent)) {
                    e->senses[i].examples[0].ex_sent_count = cJSON_GetArraySize(ex_sent);
                    for (int j = 0; j < e->senses[i].examples[0].ex_sent_count; j++) {
                        cJSON* s = cJSON_GetArrayItem(ex_sent, j);
                        if (s && cJSON_IsString(s)) {
                            strncpy(e->senses[i].examples[0].ex_sent[j], s->valuestring, sizeof(e->senses[i].examples[0].ex_sent[j])-1);
                        }
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    set_console_utf8(); // Configura la consola para UTF-8 en Windows
#endif
    const char* db_path = "./tango.db"; // Cambia esto al path correcto de tu base de datos
    TangoDB* db = tango_db_open(db_path);
    if (!db) {
        fprintf(stderr, "Error al abrir la base de datos.\n");
        return EXIT_FAILURE;
    }


    entry entry[1000];
    printf("size of entry array: %zu\n", sizeof(entry));

    return EXIT_SUCCESS;
}