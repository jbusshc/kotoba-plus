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

typedef struct TangoDB {
    struct sqlite3 *db;
    struct sqlite3_stmt *stmt;
    srs_stats stats;

} TangoDB;


TangoDB* tango_db_open(const char* db_path) {
    TangoDB* db = (TangoDB*)malloc(sizeof(TangoDB));
    if (!db) return NULL;

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

    db->stmt = NULL;
    return db;
}

void tango_db_close(TangoDB* db) {
    if (db) {
        if (db->stmt) {
            sqlite3_finalize(db->stmt);
        }
        sqlite3_close(db->db);
        free(db);
    }
}

void tango_db_warmup(TangoDB* db) {
    const char* sql = 
        "SELECT entry_id FROM entry_search "
        "WHERE entry_search MATCH 'a*' LIMIT 1;";

    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Warmup prepare failed: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    // Ejecutar aunque no devuelva resultados
    sqlite3_step(stmt);

    sqlite3_finalize(stmt);
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
        printf("Found sense array with %d elements\n", cJSON_GetArraySize(sense));
        e->senses_count = cJSON_GetArraySize(sense);
        printf("Processing %d senses\n", e->senses_count);
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
                    strncpy(e->senses[i].examples[e->senses[i].examples_count].ex_srce, ex_srce->valuestring, sizeof(e->senses[i].examples[0].ex_srce)-1);
                }
                cJSON* ex_text = cJSON_GetObjectItem(example, "ex_text");
                if (ex_text && cJSON_IsString(ex_text)) {
                    strncpy(e->senses[i].examples[e->senses[i].examples_count].ex_text, ex_text->valuestring, sizeof(e->senses[i].examples[0].ex_text)-1);
                }
                cJSON* ex_sent = cJSON_GetObjectItem(example, "ex_sent");
                if (ex_sent && cJSON_IsArray(ex_sent)) {
                    e->senses[i].examples[e->senses[i].examples_count].ex_sent_count = cJSON_GetArraySize(ex_sent);
                    for (int j = 0; j < e->senses[i].examples[e->senses[i].examples_count].ex_sent_count; j++) {
                        cJSON* s = cJSON_GetArrayItem(ex_sent, j);
                        if (s && cJSON_IsString(s)) {
                            strncpy(e->senses[i].examples[e->senses[i].examples_count].ex_sent[j], s->valuestring, sizeof(e->senses[i].examples[0].ex_sent[j])-1);
                        }
                    }
                }
                e->senses[i].examples_count++;
            }
        }
    }
    cJSON_Delete(root);
}

/* 
DB Schema

CREATE TABLE entries (
    id INTEGER PRIMARY KEY,
    priority INTEGER DEFAULT 9999,
    entry_json TEXT NOT NULL  -- JSON con { "k_ele": [...], "r_ele": [...], "sense": [...] }
);


CREATE VIRTUAL TABLE entry_search USING fts5(
    entry_id UNINDEXED,
    priority UNINDEXED,  -- usada solo para ordenamiento en consultas
    kanji,
    reading,
    gloss,
    tokenize = "unicode61"
);

CREATE TABLE IF NOT EXISTS srs_reviews (
    entry_id INTEGER NOT NULL,
    last_review INTEGER NOT NULL,   -- Unix timestamp en segundos
    interval INTEGER NOT NULL,      -- días para la próxima revisión
    ease_factor REAL NOT NULL,      -- factor de facilidad, ejemplo 2.5 inicial
    repetitions INTEGER NOT NULL,   -- cuántas veces se repasó
    due_date INTEGER NOT NULL,      -- Unix timestamp para la siguiente revisión

    PRIMARY KEY (entry_id)
);

*/

void tango_db_text_search(TangoDB* db, const char* termino, TangoSearchResultCallback callback, void* userdata) {
    const char* sql =
        "SELECT entry_id, priority, kanji, reading, gloss "
        "FROM entry_search "
        "WHERE entry_search MATCH ? OR entry_search MATCH ? OR entry_search MATCH ? "
        "ORDER BY priority ASC LIMIT 20;";

    sqlite3_stmt* stmt;

    char termino_wildcard[260];
    char hiragana_wildcard[260];
    char katakana_wildcard[260];

    // Copia y convierte
    snprintf(termino_wildcard, sizeof(termino_wildcard), "%s", termino);
    romaji_mixed_to_kana(termino, hiragana_wildcard, 0);
    romaji_mixed_to_kana(termino, katakana_wildcard, 1);
    snprintf(hiragana_wildcard, sizeof(hiragana_wildcard), "%s*", hiragana_wildcard);
    snprintf(katakana_wildcard, sizeof(katakana_wildcard), "%s*", katakana_wildcard);

    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_text(stmt, 1, termino_wildcard, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hiragana_wildcard, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, katakana_wildcard, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TangoSearchResult result = {0};
        result.ent_seq = sqlite3_column_int(stmt, 0);
        result.priority = sqlite3_column_int(stmt, 1);
        result.kanjis = (const char*)sqlite3_column_text(stmt, 2);
        result.readings = (const char*)sqlite3_column_text(stmt, 3);
        result.glosses = (const char*)sqlite3_column_text(stmt, 4);
        if (callback) {
            callback(&result, userdata);
        }
    }
    sqlite3_finalize(stmt);
}


void tango_db_id_search(TangoDB* db, int id, entry* e, TangoEntryCallback callback, void* userdata) {
    const char* sql =
        "SELECT id, priority, entry_json "
        "FROM entries "
        "WHERE id = ?;";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return;
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

    sqlite3_finalize(stmt);
}

void tango_update_srs_review(TangoDB* Tdb, int entry_id, int quality) {
    if (quality < 0) quality = 0;
    if (quality > 5) quality = 5;

    time_t now = time(NULL);

    // Consulta estado actual
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(Tdb->db,
        "SELECT last_review, interval, ease_factor, repetitions FROM srs_reviews WHERE entry_id = ?",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int(stmt, 1, entry_id);

    time_t last_review = 0;
    int interval = 0;
    double ease_factor = 2.5;
    int repetitions = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        last_review = (time_t)sqlite3_column_int64(stmt, 0);
        interval = sqlite3_column_int(stmt, 1);
        ease_factor = sqlite3_column_double(stmt, 2);
        repetitions = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);

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

    // Insertar o actualizar
    rc = sqlite3_prepare_v2(Tdb->db,
        "INSERT INTO srs_reviews (entry_id, last_review, interval, ease_factor, repetitions, due_date) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(entry_id) DO UPDATE SET "
        "last_review=excluded.last_review, interval=excluded.interval, ease_factor=excluded.ease_factor, repetitions=excluded.repetitions, due_date=excluded.due_date;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK) return;

    sqlite3_bind_int(stmt, 1, entry_id);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int(stmt, 3, interval);
    sqlite3_bind_double(stmt, 4, ease_factor);
    sqlite3_bind_int(stmt, 5, repetitions);
    sqlite3_bind_int64(stmt, 6, due_date);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    // Actualizar estadísticas
    tango_update_srs_stats(Tdb);
}

int tango_get_total_cards(TangoDB* db) {
    if (!db) return 0;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM entries;";
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }

    int total_cards = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total_cards = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total_cards;
}

int tango_get_due_review_count(TangoDB* db) {
    if (!db) return 0;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM srs_reviews WHERE due_date <= ?;";
    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, time(NULL));

    int due_count = 0;
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

    const char* sql =
        "SELECT entry_id, last_review, interval, ease_factor, repetitions, due_date "
        "FROM srs_reviews "
        "ORDER BY due_date ASC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt;

    // Calculamos desde dónde empezar en la tabla, para evitar duplicar cartas ya en buffer
    int offset = buffer->start_index + buffer->count;

    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta SRS: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    if (sqlite3_bind_int(stmt, 1, cards_to_add) != SQLITE_OK ||
        sqlite3_bind_int(stmt, 2, offset) != SQLITE_OK) {
        fprintf(stderr, "❌ Error al vincular parámetros: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return;
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
                // La carta actual fue sobrescrita, puedes decidir:
                // - Mover current_index a 0 (la nueva carta más vieja)
                // - O invalidar current_index (ej: -1)
                buffer->current_index = 0;
            } else {
                // Ajustar current_index para mantener coherencia
                buffer->current_index--;
            }
        }
        added++;
    }

    sqlite3_finalize(stmt);
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

    const char* sql = "SELECT COUNT(*) FROM srs_reviews WHERE due_date <= ?;";
    sqlite3_stmt* stmt;
    int count = 0;

    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Error en consulta: %s\n", sqlite3_errmsg(db->db));
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, time(NULL));

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

srs_stats* tango_get_srs_stats(TangoDB* db) {
    if (!db) return NULL;

    // Actualizar estadísticas
    tango_update_srs_stats(db);
    

    return &db->stats;
}