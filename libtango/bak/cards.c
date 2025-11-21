
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

void add_card(sqlite3* db, int entry_id, const char* card_type) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO cards (entry_id, card_type, repetitions, ease_factor, interval, last_reviewed, due_date, status) "
                      "VALUES (?, ?, 0, 2.5, 1, strftime('%s','now'), strftime('%s','now'), 'active');";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, entry_id);
    sqlite3_bind_text(stmt, 2, card_type, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("✅ Tarjeta añadida para entry_id %d (%s)\n", entry_id, card_type);
    } else {
        fprintf(stderr, "Error añadiendo tarjeta: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

void suspend_card(sqlite3* db, int card_id) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE cards SET status = 'suspended' WHERE id = ?;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, card_id);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("🛑 Tarjeta %d suspendida.\n", card_id);
    } else {
        fprintf(stderr, "Error suspendiendo tarjeta: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

void activate_card(sqlite3* db, int card_id) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE cards SET status = 'active' WHERE id = ?;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, card_id);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("✅ Tarjeta %d reactivada.\n", card_id);
    } else {
        fprintf(stderr, "Error reactivando tarjeta: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

void delete_card(sqlite3* db, int card_id) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM cards WHERE id = ?;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, card_id);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("🗑️  Tarjeta %d eliminada.\n", card_id);
    } else {
        fprintf(stderr, "Error eliminando tarjeta: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

void list_cards(sqlite3* db) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT cards.id, cards.entry_id, card_type, repetitions, interval, due_date, status, "
        "(SELECT GROUP_CONCAT(keb, ', ') FROM kanjis WHERE entry_id = cards.entry_id), "
        "(SELECT GROUP_CONCAT(gloss, ', ') FROM glosses "
        " JOIN senses ON glosses.sense_id = senses.id WHERE senses.entry_id = cards.entry_id) "
        "FROM cards;";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    printf("\n📋 Tarjetas:\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int entry_id = sqlite3_column_int(stmt, 1);
        const char* type = (const char*)sqlite3_column_text(stmt, 2);
        int reps = sqlite3_column_int(stmt, 3);
        int interval = sqlite3_column_int(stmt, 4);
        time_t due = sqlite3_column_int64(stmt, 5);
        const char* status = (const char*)sqlite3_column_text(stmt, 6);
        const char* kanji = (const char*)sqlite3_column_text(stmt, 7);
        const char* gloss = (const char*)sqlite3_column_text(stmt, 8);

        char due_date[64];
        strftime(due_date, sizeof(due_date), "%Y-%m-%d", localtime(&due));

        printf("ID %d | Entrada %d | Tipo %s | %s [%s] | %d rep | Prox: %s | Estado: %s\n",
            id, entry_id, type, kanji ? kanji : "", gloss ? gloss : "", reps, due_date, status);
    }
    sqlite3_finalize(stmt);
}
/*
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso:\n");
        printf("  %s add <entry_id> <tipo>     Añade una tarjeta\n", argv[0]);
        printf("  %s suspend <card_id>         Suspende una tarjeta\n", argv[0]);
        printf("  %s activate <card_id>        Reactiva una tarjeta\n", argv[0]);
        printf("  %s delete <card_id>          Elimina una tarjeta\n", argv[0]);
        printf("  %s list                      Lista todas las tarjetas\n", argv[0]);
        return 0;
    }

    sqlite3 *db;
    if (sqlite3_open("jmdict.db", &db)) {
        fprintf(stderr, "No se pudo abrir jmdict.db: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    if (strcmp(argv[1], "add") == 0 && argc == 4) {
        int entry_id = atoi(argv[2]);
        add_card(db, entry_id, argv[3]);
    } else if (strcmp(argv[1], "suspend") == 0 && argc == 3) {
        int card_id = atoi(argv[2]);
        suspend_card(db, card_id);
    } else if (strcmp(argv[1], "activate") == 0 && argc == 3) {
        int card_id = atoi(argv[2]);
        activate_card(db, card_id);
    } else if (strcmp(argv[1], "delete") == 0 && argc == 3) {
        int card_id = atoi(argv[2]);
        delete_card(db, card_id);
    } else if (strcmp(argv[1], "list") == 0) {
        list_cards(db);
    } else {
        fprintf(stderr, "Comando no válido.\n");
    }

    sqlite3_close(db);
    return 0;
}
*/