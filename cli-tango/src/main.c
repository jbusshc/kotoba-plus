#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libtango.h"

#include <cjson/cJSON.h>
#include <sqlite3.h>

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

void test (entry* e) {
    if (!e)  {
        printf("Entrada nula\n");
        return;
    }
    for (int i = 0; i < e->r_elements_count; i++) {
        if (e->r_elements[i].re_nokanji) {
            printf("Entrada %d tiene reb sin kanji:\n", e->ent_seq);
            printf("Reb %d: %s, nokanji: %d\n", i, e->r_elements[i].reb, e->r_elements[i].re_nokanji);
        }
    }
}


int main(int argc, char* argv[]) {
#ifdef _WIN32
    set_console_utf8(); // Configura la consola para UTF-8 en Windows
#endif
    const char* db_path = "tango.db"; // Cambia esto al path correcto de tu base de datos

    const char* sql = "SELECT id from entries";
    
    sqlite3* sqlite_db;
    if (sqlite3_open(db_path, &sqlite_db) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos SQLite: %s\n", sqlite3_errmsg(sqlite_db));
        return EXIT_FAILURE;
    }
    TangoDB* db = tango_db_open(db_path);
    if (!db) {
        fprintf(stderr, "No se pudo abrir la base de datos Tango.\n");
        sqlite3_close(sqlite_db);
        return EXIT_FAILURE;
    }
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error al preparar la consulta: %s\n", sqlite3_errmsg(sqlite_db));
        return EXIT_FAILURE;
    }
    

    printf("Analizando entradas para estadísticas máximas...\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int entry_id = sqlite3_column_int(stmt, 0);
        
        entry e;
        memset(&e, 0, sizeof(entry));
        
        tango_db_id_search(db, entry_id, &e, NULL, NULL);
        test(&e);
    }

    sqlite3_finalize(stmt);
    tango_db_close(db);


    return EXIT_SUCCESS;
}

