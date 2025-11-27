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

// Ejecuta una consulta SQL y muestra cada fila y columna
static void run_query(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt;

    printf("\n> SQL: %s\n", sql);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Error preparando statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    int col_count = sqlite3_column_count(stmt);

    // Procesar filas
    while (1) {
        int rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW) {
            // Mostrar columnas
            for (int i = 0; i < col_count; i++) {
                const unsigned char *val = sqlite3_column_text(stmt, i);
                printf("%s%s", val ? (const char*)val : "(null)",
                       (i == col_count - 1 ? "" : " | "));
            }
            printf("\n");
        }
        else if (rc == SQLITE_DONE) {
            break;
        }
        else {
            printf("Error en step: %s\n", sqlite3_errmsg(db));
            break;
        }
    }

    sqlite3_finalize(stmt);
}


int main(int argc, char* argv[]) {
#ifdef _WIN32
    set_console_utf8(); // Configura la consola para UTF-8 en Windows
#endif
    const char* db_path = "tango.db"; // Cambia esto al path correcto de tu base de datos
    TangoDB* db = tango_db_open(db_path);
    if (!db) {
        fprintf(stderr, "Error al abrir la base de datos.\n");
        return EXIT_FAILURE;
    }

    // Realizar algunas consultas de ejemplo
    sqlite3* db2 = NULL;
    if (sqlite3_open(db_path, &db2) != SQLITE_OK) {
        fprintf(stderr, "No se pudo abrir la base de datos con sqlite3: %s\n", sqlite3_errmsg(db2));
        return EXIT_FAILURE;
    }

    // Consulta todos los IDs de la tabla entries y muestra cada entrada
    const char *sql = "SELECT id FROM entries";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db2, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparando statement: %s\n", sqlite3_errmsg(db2));
    } else {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int ent_seq = sqlite3_column_int(stmt, 0);
            entry e;
            tango_db_id_search(db, ent_seq, &e, imprimir_entry, NULL);
        }
        sqlite3_finalize(stmt);
    }
    tango_db_close(db);


    return EXIT_SUCCESS;
}

