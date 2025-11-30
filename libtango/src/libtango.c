// libtango.c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <math.h>
#include <cjson/cJSON.h>
#include <stdint.h>
#include "libtango.h"
#include "kana-converter.h"

typedef struct srs_stats{
    int total_cards;
    int due_review_count;
    time_t last_update;
} srs_stats;

typedef struct {
    sqlite3_stmt* stmt_get_entry;
    sqlite3_stmt* stmt_search;

    // nuevos prepared statements
    sqlite3_stmt* stmt_count_entries;
    sqlite3_stmt* stmt_count_due;
    sqlite3_stmt* stmt_srs_select; // SELECT last_review, interval, ease_factor, repetitions FROM srs_reviews WHERE entry_id = ?
    sqlite3_stmt* stmt_srs_upsert; // INSERT ... ON CONFLICT ... DO UPDATE ...
    sqlite3_stmt* stmt_srs_page;   // SELECT ... FROM srs_reviews ORDER BY due_date ASC LIMIT ? OFFSET ?
} TangoStatements;

typedef struct TangoDB {
    sqlite3 *db;
    TangoStatements stmts;
    srs_stats stats;
} TangoDB;

#define SEP '\x1F'  // Separador ASCII Unit Separator para campos combinados
#ifndef BUFFER_LIMIT
#define BUFFER_LIMIT 256  // si no está definido en header, ajustar según tu header real
#endif

/* ---------- Helpers ---------- */

TangoDB* tango_db_open(const char* db_path) {
    TangoDB* db = (TangoDB*)malloc(sizeof(TangoDB));
    if (!db) return NULL;

    memset(db, 0, sizeof(TangoDB)); // inicializa todo a 0/NULL

    if (sqlite3_open(db_path, &db->db) != SQLITE_OK) {
        free(db);
        return NULL;
    }

    // Activar modo WAL para mejor concurrencia y rendimiento
    char* errmsg = NULL;
    if (sqlite3_exec(db->db, "PRAGMA journal_mode=WAL;", NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "Error activando WAL: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }

    // Inicializar pointers de statements a NULL (ya lo hizo el memset, repetimos por claridad)
    db->stmts.stmt_get_entry = NULL;
    db->stmts.stmt_search = NULL;
    db->stmts.stmt_count_entries = NULL;
    db->stmts.stmt_count_due = NULL;
    db->stmts.stmt_srs_select = NULL;
    db->stmts.stmt_srs_upsert = NULL;
    db->stmts.stmt_srs_page = NULL;

    tango_db_warmup(db);

    return db;
}

void _finalize_if(sqlite3_stmt** p) {
    if (p && *p) {
        sqlite3_finalize(*p);
        *p = NULL;
    }
}

void tango_db_close(TangoDB* db) {
    if (!db) return;

    // Finalizar prepared statements si existen
    _finalize_if(&db->stmts.stmt_get_entry);
    _finalize_if(&db->stmts.stmt_search);
    _finalize_if(&db->stmts.stmt_count_entries);
    _finalize_if(&db->stmts.stmt_count_due);
    _finalize_if(&db->stmts.stmt_srs_select);
    _finalize_if(&db->stmts.stmt_srs_upsert);
    _finalize_if(&db->stmts.stmt_srs_page);

    // Cerrar DB
    if (db->db) {
        sqlite3_close(db->db);
        db->db = NULL;
    }

    free(db);
}

static void warmup_simple(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_step(stmt);  // no importa si no devuelve nada
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Warmup prepare failed for '%s': %s\n",
            sql, sqlite3_errmsg(db));
    }
}

// Opcional: preparar statements usados frecuentemente
static void prepare_critical_statements(TangoDB* db) {
    if (!db || !db->db) return;

    int rc;

    // cargar una entrada por ID
    const char* sql_get_entry =
        "SELECT id, priority, entry_json FROM entries WHERE id = ?;";
    rc = sqlite3_prepare_v2(db->db, sql_get_entry, -1,
                           &db->stmts.stmt_get_entry, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_get_entry: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_get_entry = NULL;
    }

    // búsqueda en FTS
    const char* sql_search =
        "SELECT entry_id, priority, content "
        "FROM entry_search "
        "WHERE content MATCH ? OR content MATCH ? OR content MATCH ? "
        "ORDER BY priority ASC LIMIT 20;";
    rc = sqlite3_prepare_v2(db->db, sql_search, -1,
                           &db->stmts.stmt_search, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_search: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_search = NULL;
    }

    // COUNT(*) FROM entries
    const char* sql_count_entries = "SELECT COUNT(*) FROM entries;";
    rc = sqlite3_prepare_v2(db->db, sql_count_entries, -1,
                           &db->stmts.stmt_count_entries, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_count_entries: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_count_entries = NULL;
    }

    // COUNT(*) FROM srs_reviews WHERE due_date <= ?
    const char* sql_count_due = "SELECT COUNT(*) FROM srs_reviews WHERE due_date <= ?;";
    rc = sqlite3_prepare_v2(db->db, sql_count_due, -1,
                           &db->stmts.stmt_count_due, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_count_due: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_count_due = NULL;
    }

    // SELECT srs row by entry_id
    const char* sql_srs_select = "SELECT last_review, interval, ease_factor, repetitions FROM srs_reviews WHERE entry_id = ?;";
    rc = sqlite3_prepare_v2(db->db, sql_srs_select, -1,
                           &db->stmts.stmt_srs_select, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_srs_select: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_srs_select = NULL;
    }

    // UPSERT for srs_reviews
    const char* sql_srs_upsert =
        "INSERT INTO srs_reviews (entry_id, last_review, interval, ease_factor, repetitions, due_date) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(entry_id) DO UPDATE SET "
        "last_review=excluded.last_review, interval=excluded.interval, ease_factor=excluded.ease_factor, repetitions=excluded.repetitions, due_date=excluded.due_date;";
    rc = sqlite3_prepare_v2(db->db, sql_srs_upsert, -1,
                           &db->stmts.stmt_srs_upsert, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_srs_upsert: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_srs_upsert = NULL;
    }

    // Page query for srs_reviews: LIMIT ? OFFSET ?
    const char* sql_srs_page =
        "SELECT entry_id, last_review, interval, ease_factor, repetitions, due_date "
        "FROM srs_reviews "
        "ORDER BY due_date ASC LIMIT ? OFFSET ?;";
    rc = sqlite3_prepare_v2(db->db, sql_srs_page, -1,
                           &db->stmts.stmt_srs_page, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare stmt_srs_page: %s\n",
                sqlite3_errmsg(db->db));
        db->stmts.stmt_srs_page = NULL;
    }
}

void tango_db_warmup(TangoDB* db) {
    if (!db || !db->db) return;

    // ========================================
    // 1. PRAGMAs de performance (solo una vez)
    // ========================================
    sqlite3_exec(db->db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA temp_store=MEMORY;", NULL, NULL, NULL);
    sqlite3_exec(db->db, "PRAGMA foreign_keys=ON;", NULL, NULL, NULL);

    // 256 MB de mmap si se puede
    sqlite3_exec(db->db, "PRAGMA mmap_size=268435456;", NULL, NULL, NULL);

    // ========================================
    // 2. Warmup de metadatos del esquema
    // ========================================
    warmup_simple(db->db, "PRAGMA schema_version;");

    // ========================================
    // 3. Warmup de la tabla base
    // ========================================
    warmup_simple(db->db, "SELECT id FROM entries LIMIT 1;");

    // ========================================
    // 4. Warmup del índice FTS5
    // (obliga a cargar la root page del índice FTS)
    // ========================================
    warmup_simple(db->db,
        "SELECT entry_id FROM entry_search "
        "WHERE entry_search MATCH 'a*' LIMIT 1;"
    );

    // ========================================
    // 5. Pre-preparar statements críticos
    // ========================================
    prepare_critical_statements(db);
}

/* ---------- JSON parsing (tu función parse_entry_json) ---------- */

void parse_entry_json(const char* json, entry* e) {
    cJSON* root = cJSON_Parse(json);
    if (!root) {
        printf("Failed to parse JSON: %s\n", json);
        return;
    }

    // ================================
    // k_ele
    // ================================
    cJSON* k_ele = cJSON_GetObjectItem(root, "k_ele");
    if (k_ele && cJSON_IsArray(k_ele)) {
        e->k_elements_count = cJSON_GetArraySize(k_ele);

        for (int i = 0; i < e->k_elements_count; i++) {
            cJSON* obj = cJSON_GetArrayItem(k_ele, i);
            if (!obj) continue;

            // keb
            cJSON* keb = cJSON_GetObjectItem(obj, "keb");
            if (keb && cJSON_IsString(keb)) {
                strncpy(e->k_elements[i].keb, keb->valuestring, sizeof(e->k_elements[i].keb) - 1);
            }

            // ke_inf
            cJSON* ke_inf = cJSON_GetObjectItem(obj, "ke_inf");
            if (ke_inf && cJSON_IsArray(ke_inf)) {
                e->k_elements[i].ke_inf_count = cJSON_GetArraySize(ke_inf);
                for (int j = 0; j < e->k_elements[i].ke_inf_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(ke_inf, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->k_elements[i].ke_inf[j], s->valuestring, sizeof(e->k_elements[i].ke_inf[j]) - 1);
                    }
                }
            }

            // ke_pri
            cJSON* ke_pri = cJSON_GetObjectItem(obj, "ke_pri");
            if (ke_pri && cJSON_IsArray(ke_pri)) {
                e->k_elements[i].ke_pri_count = cJSON_GetArraySize(ke_pri);
                for (int j = 0; j < e->k_elements[i].ke_pri_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(ke_pri, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->k_elements[i].ke_pri[j], s->valuestring, sizeof(e->k_elements[i].ke_pri[j]) - 1);
                    }
                }
            }
        }
    }

    // ================================
    // r_ele
    // ================================
    cJSON* r_ele = cJSON_GetObjectItem(root, "r_ele");
    if (r_ele && cJSON_IsArray(r_ele)) {
        e->r_elements_count = cJSON_GetArraySize(r_ele);

        for (int i = 0; i < e->r_elements_count; i++) {
            cJSON* obj = cJSON_GetArrayItem(r_ele, i);
            if (!obj) continue;

            // reb
            cJSON* reb = cJSON_GetObjectItem(obj, "reb");
            if (reb && cJSON_IsString(reb)) {
                strncpy(e->r_elements[i].reb, reb->valuestring, sizeof(e->r_elements[i].reb) - 1);
            }

            // re_nokanji
            cJSON* re_nokanji = cJSON_GetObjectItem(obj, "re_nokanji");
            if (re_nokanji && cJSON_IsString(re_nokanji)) {
                strncpy(e->r_elements[i].re_nokanji, re_nokanji->valuestring, sizeof(e->r_elements[i].re_nokanji) - 1);
            }

            // re_restr
            cJSON* re_restr = cJSON_GetObjectItem(obj, "re_restr");
            if (re_restr && cJSON_IsArray(re_restr)) {
                e->r_elements[i].re_restr_count = cJSON_GetArraySize(re_restr);
                for (int j = 0; j < e->r_elements[i].re_restr_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(re_restr, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->r_elements[i].re_restr[j], s->valuestring, sizeof(e->r_elements[i].re_restr[j]) - 1);
                    }
                }
            }

            // re_inf
            cJSON* re_inf = cJSON_GetObjectItem(obj, "re_inf");
            if (re_inf && cJSON_IsArray(re_inf)) {
                e->r_elements[i].re_inf_count = cJSON_GetArraySize(re_inf);
                for (int j = 0; j < e->r_elements[i].re_inf_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(re_inf, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->r_elements[i].re_inf[j], s->valuestring, sizeof(e->r_elements[i].re_inf[j]) - 1);
                    }
                }
            }

            // re_pri
            cJSON* re_pri = cJSON_GetObjectItem(obj, "re_pri");
            if (re_pri && cJSON_IsArray(re_pri)) {
                e->r_elements[i].re_pri_count = cJSON_GetArraySize(re_pri);
                for (int j = 0; j < e->r_elements[i].re_pri_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(re_pri, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->r_elements[i].re_pri[j], s->valuestring, sizeof(e->r_elements[i].re_pri[j]) - 1);
                    }
                }
            }
        }
    }

    // ================================
    // sense
    // ================================
    cJSON* sense = cJSON_GetObjectItem(root, "sense");
    if (sense && cJSON_IsArray(sense)) {
        e->senses_count = cJSON_GetArraySize(sense);

        for (int i = 0; i < e->senses_count; i++) {
            cJSON* sobj = cJSON_GetArrayItem(sense, i);
            if (!sobj) continue;

            // pos
            cJSON* pos = cJSON_GetObjectItem(sobj, "pos");
            if (pos && cJSON_IsArray(pos)) {
                e->senses[i].pos_count = cJSON_GetArraySize(pos);
                for (int j = 0; j < e->senses[i].pos_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(pos, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->senses[i].pos[j], s->valuestring, sizeof(e->senses[i].pos[j]) - 1);
                    }
                }
            }

            // gloss
            cJSON* gloss = cJSON_GetObjectItem(sobj, "gloss");
            if (gloss && cJSON_IsArray(gloss)) {
                e->senses[i].gloss_count = cJSON_GetArraySize(gloss);
                for (int j = 0; j < e->senses[i].gloss_count; j++) {
                    cJSON* s = cJSON_GetArrayItem(gloss, j);
                    if (s && cJSON_IsString(s)) {
                        strncpy(e->senses[i].gloss[j], s->valuestring, sizeof(e->senses[i].gloss[j]) - 1);
                    }
                }
            }

            // ======================
            // example (ARREGLO)
            // ======================
            cJSON* example = cJSON_GetObjectItem(sobj, "example");
            if (example && cJSON_IsArray(example)) {

                e->senses[i].examples_count = cJSON_GetArraySize(example);

                for (int ex = 0; ex < e->senses[i].examples_count; ex++) {

                    cJSON* exobj = cJSON_GetArrayItem(example, ex);
                    if (!exobj) continue;

                    // ex_srce
                    cJSON* ex_srce = cJSON_GetObjectItem(exobj, "ex_srce");
                    if (ex_srce && cJSON_IsString(ex_srce)) {
                        strncpy(e->senses[i].examples[ex].ex_srce,
                                ex_srce->valuestring,
                                sizeof(e->senses[i].examples[ex].ex_srce) - 1);
                    }

                    // ex_text
                    cJSON* ex_text = cJSON_GetObjectItem(exobj, "ex_text");
                    if (ex_text && cJSON_IsString(ex_text)) {
                        strncpy(e->senses[i].examples[ex].ex_text,
                                ex_text->valuestring,
                                sizeof(e->senses[i].examples[ex].ex_text) - 1);
                    }

                    // ex_sent (array)
                    cJSON* ex_sent = cJSON_GetObjectItem(exobj, "ex_sent");
                    if (ex_sent && cJSON_IsArray(ex_sent)) {

                        e->senses[i].examples[ex].ex_sent_count =
                            cJSON_GetArraySize(ex_sent);

                        for (int k = 0; k < e->senses[i].examples[ex].ex_sent_count; k++) {
                            cJSON* s = cJSON_GetArrayItem(ex_sent, k);
                            if (s && cJSON_IsString(s)) {
                                strncpy(e->senses[i].examples[ex].ex_sent[k],
                                        s->valuestring,
                                        sizeof(e->senses[i].examples[ex].ex_sent[k]) - 1);
                            }
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
}

/* ---------- Consultas y funciones públicas ---------- */

void tango_db_text_search(TangoDB* db, const char* termino, TangoSearchResultCallback callback, void* userdata) {
    if (!db || !db->db || !termino) return;

    sqlite3_stmt* stmt = NULL;
    int using_prepared = 0;

    char termino_wildcard[260];
    char hiragana_wildcard[260];
    char katakana_wildcard[260];

    // Copia y convierte
    snprintf(termino_wildcard, sizeof(termino_wildcard), "%s", termino);
    romaji_mixed_to_kana(termino, hiragana_wildcard, 0);
    romaji_mixed_to_kana(termino, katakana_wildcard, 1);
    snprintf(hiragana_wildcard, sizeof(hiragana_wildcard), "%s*", hiragana_wildcard);
    snprintf(katakana_wildcard, sizeof(katakana_wildcard), "%s*", katakana_wildcard);

    if (db->stmts.stmt_search) {
        stmt = db->stmts.stmt_search;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        using_prepared = 1;
    }

    if (!using_prepared) {
        const char* sql =
            "SELECT entry_id, priority, content "
            "FROM entry_search "
            "WHERE content MATCH ? OR content MATCH ? OR content MATCH ? "
            "ORDER BY priority ASC LIMIT 20;";
        if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
            return;
        }
    }

    sqlite3_bind_text(stmt, 1, termino_wildcard, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hiragana_wildcard, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, katakana_wildcard, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TangoSearchResult result = {0};
        result.ent_seq = sqlite3_column_int(stmt, 0);
        result.priority = sqlite3_column_int(stmt, 1);

        const unsigned char* content = sqlite3_column_text(stmt, 2);
        if (!content) continue;

        const char sep = SEP;

        char buffer[2048];
        snprintf(buffer, sizeof(buffer), "%s", content);

        char* k = buffer;
        char* r = strchr(k, sep);
        char* g = NULL;

        if (r) {
            *r++ = '\0';
            g = strchr(r, sep);
            if (g) {
                *g++ = '\0';
            }
        }

        if (!k) k = "";
        if (!r) r = "";
        if (!g) g = "";

        result.kanjis   = k;
        result.readings = r;
        result.glosses  = g;

        if (callback) {
            callback(&result, userdata);
        }
    }

    if (!using_prepared) sqlite3_finalize(stmt);
    else {
        // leave the prepared statement finalized at close time; reset is enough
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
}

void tango_db_id_search(TangoDB* db, int id, entry* e, TangoEntryCallback callback, void* userdata) {
    if (!db || !db->db || !e) return;

    sqlite3_stmt* stmt = NULL;
    int using_prepared = 0;

    if (db->stmts.stmt_get_entry) {
        stmt = db->stmts.stmt_get_entry;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        using_prepared = 1;
    }

    if (!using_prepared) {
        const char* sql =
            "SELECT id, priority, entry_json "
            "FROM entries "
            "WHERE id = ?;";
        if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
            return;
        }
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        e->ent_seq = sqlite3_column_int(stmt, 0);
        e->priority = sqlite3_column_int(stmt, 1);
        const char* entry_json = (const char*)sqlite3_column_text(stmt, 2);

        if (entry_json) {
            parse_entry_json(entry_json, e );
        }
        if (callback) {
            callback(e, userdata);
        }
    }

    if (!using_prepared) sqlite3_finalize(stmt);
    else {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
}

void tango_update_srs_review(TangoDB* Tdb, int entry_id, int quality) {
    if (!Tdb || !Tdb->db) return;
    if (quality < 0) quality = 0;
    if (quality > 5) quality = 5;

    time_t now = time(NULL);

    // Valores por defecto
    time_t last_review = 0;
    int interval = 0;
    double ease_factor = 2.5;
    int repetitions = 0;

    // Intentar usar stmt_srs_select preparado
    if (Tdb->stmts.stmt_srs_select) {
        sqlite3_stmt* s = Tdb->stmts.stmt_srs_select;
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
        sqlite3_bind_int(s, 1, entry_id);

        if (sqlite3_step(s) == SQLITE_ROW) {
            last_review = (time_t)sqlite3_column_int64(s, 0);
            interval = sqlite3_column_int(s, 1);
            ease_factor = sqlite3_column_double(s, 2);
            repetitions = sqlite3_column_int(s, 3);
        }
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
    } else {
        // Fallback: preparar localmente y ejecutar si no hay statement preparado
        sqlite3_stmt* stmt = NULL;
        int rc = sqlite3_prepare_v2(Tdb->db,
            "SELECT last_review, interval, ease_factor, repetitions FROM srs_reviews WHERE entry_id = ?",
            -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, entry_id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                last_review = (time_t)sqlite3_column_int64(stmt, 0);
                interval = sqlite3_column_int(stmt, 1);
                ease_factor = sqlite3_column_double(stmt, 2);
                repetitions = sqlite3_column_int(stmt, 3);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Reglas SM-2-ish
    if (quality < 3) {
        repetitions = 0;
        interval = 1;
    } else {
        if (repetitions == 0) {
            interval = 1;
        } else if (repetitions == 1) {
            interval = 6;
        } else {
            interval = (int)round(interval * ease_factor);
        }
        repetitions++;
        ease_factor = ease_factor + (0.1 - (5 - quality) * (0.08 + (5 - quality) * 0.02));
        if (ease_factor < 1.3) ease_factor = 1.3;
    }

    time_t due_date = now + interval * 86400;

    // Insertar o actualizar: usar stmt_srs_upsert si existe
    if (Tdb->stmts.stmt_srs_upsert) {
        sqlite3_stmt* s = Tdb->stmts.stmt_srs_upsert;
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);

        sqlite3_bind_int(s, 1, entry_id);
        sqlite3_bind_int64(s, 2, now);
        sqlite3_bind_int(s, 3, interval);
        sqlite3_bind_double(s, 4, ease_factor);
        sqlite3_bind_int(s, 5, repetitions);
        sqlite3_bind_int64(s, 6, due_date);

        sqlite3_step(s);
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
    } else {
        // Fallback: preparar localmente
        sqlite3_stmt* stmt = NULL;
        int rc = sqlite3_prepare_v2(Tdb->db,
            "INSERT INTO srs_reviews (entry_id, last_review, interval, ease_factor, repetitions, due_date) "
            "VALUES (?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(entry_id) DO UPDATE SET "
            "last_review=excluded.last_review, interval=excluded.interval, ease_factor=excluded.ease_factor, repetitions=excluded.repetitions, due_date=excluded.due_date;",
            -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, entry_id);
            sqlite3_bind_int64(stmt, 2, now);
            sqlite3_bind_int(stmt, 3, interval);
            sqlite3_bind_double(stmt, 4, ease_factor);
            sqlite3_bind_int(stmt, 5, repetitions);
            sqlite3_bind_int64(stmt, 6, due_date);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // Actualizar estadísticas
    tango_update_srs_stats(Tdb);
}

int tango_get_total_cards(TangoDB* db) {
    if (!db) return 0;

    int total_cards = 0;

    if (db->stmts.stmt_count_entries) {
        sqlite3_stmt* s = db->stmts.stmt_count_entries;
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
        if (sqlite3_step(s) == SQLITE_ROW) {
            total_cards = sqlite3_column_int(s, 0);
        }
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
        return total_cards;
    }

    // fallback
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM entries;";
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total_cards = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return total_cards;
}

int tango_get_due_review_count(TangoDB* db) {
    if (!db) return 0;

    int due_count = 0;

    if (db->stmts.stmt_count_due) {
        sqlite3_stmt* s = db->stmts.stmt_count_due;
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
        sqlite3_bind_int64(s, 1, time(NULL));
        if (sqlite3_step(s) == SQLITE_ROW) {
            due_count = sqlite3_column_int(s, 0);
        }
        sqlite3_reset(s);
        sqlite3_clear_bindings(s);
        return due_count;
    }

    // fallback
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM srs_reviews WHERE due_date <= ?;";
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }
    sqlite3_bind_int64(stmt, 1, time(NULL));
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        due_count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return due_count;
}

void tango_update_srs_stats(TangoDB* db) {
    if (!db) return;

    // Actualizar estadísticas
    db->stats.total_cards = tango_get_total_cards(db);
    db->stats.due_review_count = tango_get_due_review_count(db);
    db->stats.last_update = time(NULL);
}

void tango_review_card_buffer_init(TangoDB* db, review_card_buffer* buffer) {
    if (!db || !buffer) return;

    tango_update_srs_stats(db);
    
    // Limpiar el buffer
    tango_review_card_buffer_clear(buffer);

    if(db->stats.due_review_count > 0) {
        // Llenar el buffer con las cartas pendientes de revisión
        tango_review_card_buffer_fill(db, buffer, BUFFER_LIMIT);
    }
}

void tango_review_card_buffer_fill(TangoDB* db, review_card_buffer* buffer, int cards_to_add) {
    if (!db || !buffer || cards_to_add <= 0) return;

    sqlite3_stmt* stmt = NULL;
    int using_prepared = 0;

    // Calculamos desde dónde empezar en la tabla, para evitar duplicar cartas ya en buffer
    int offset = buffer->start_index + buffer->count;

    if (db->stmts.stmt_srs_page) {
        stmt = db->stmts.stmt_srs_page;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, cards_to_add);
        sqlite3_bind_int(stmt, 2, offset);
        using_prepared = 1;
    }

    if (!using_prepared) {
        const char* sql =
            "SELECT entry_id, last_review, interval, ease_factor, repetitions, due_date "
            "FROM srs_reviews "
            "ORDER BY due_date ASC LIMIT ? OFFSET ?;";
        if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "❌ Error en consulta SRS: %s\n", sqlite3_errmsg(db->db));
            return;
        }
        sqlite3_bind_int(stmt, 1, cards_to_add);
        sqlite3_bind_int(stmt, 2, offset);
    }

    int added = 0;
    while (added < cards_to_add && sqlite3_step(stmt) == SQLITE_ROW) {
        int insert_idx = (buffer->start_index + buffer->count) % BUFFER_LIMIT;
        review_card* next = &buffer->cards[insert_idx];

        next->ent_seq = sqlite3_column_int(stmt, 0);
        next->review_data.last_review = (time_t)sqlite3_column_int64(stmt, 1);
        next->review_data.interval = sqlite3_column_int(stmt, 2);
        next->review_data.ease_factor = sqlite3_column_double(stmt, 3);
        next->review_data.repetitions = sqlite3_column_int(stmt, 4);
        next->review_data.due_date = (time_t)sqlite3_column_int64(stmt, 5);

        if (buffer->count < BUFFER_LIMIT) {
            buffer->count++;
        } else {
            // Buffer lleno: avanzamos start_index para sobrescribir el elemento más viejo
            buffer->start_index = (buffer->start_index + 1) % BUFFER_LIMIT;

            // Si current_index apunta al elemento sobrescrito, ajustarlo o marcar
            if (buffer->current_index == 0) {
                buffer->current_index = 0;
            } else {
                buffer->current_index--;
            }
        }
        added++;
    }

    if (!using_prepared) {
        sqlite3_finalize(stmt);
    } else {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
}

review_card* tango_review_card_buffer_get_current(review_card_buffer* buffer) {
    if (!buffer || buffer->count == 0) return NULL;

    return &buffer->cards[buffer->current_index];
}

void tango_review_card_buffer_advance(review_card_buffer* buffer) {
    if (!buffer || buffer->count == 0) return;

    buffer->current_index = (buffer->current_index + 1) % buffer->count;
}

void tango_review_card_buffer_undo(review_card_buffer* buffer) {
    if (!buffer || buffer->count == 0) return;

    buffer->current_index = (buffer->current_index + buffer->count - 1) % buffer->count;
}

void tango_review_card_buffer_clear(review_card_buffer* buffer) {
    if (!buffer) return;
    buffer->count = 0;
    buffer->start_index = 0;
    buffer->current_index = 0;
}

int tango_get_pending_review_cards(TangoDB* db) {
    if (!db) return 0;

    // Reuse tango_get_due_review_count (already uses prepared stmt if available)
    return tango_get_due_review_count(db);
}

srs_stats* tango_get_srs_stats(TangoDB* db) {
    if (!db) return NULL;

    // Actualizar estadísticas
    tango_update_srs_stats(db);

    return &db->stats;
}
