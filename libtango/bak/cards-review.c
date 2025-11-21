#include "cards-review.h"

void update_card(Card* card, int quality) {
    if (quality < 3) {
        card->repetitions = 0;
        card->interval = 1;
    } else {
        card->repetitions++;
        if (card->repetitions == 1) {
            card->interval = 1;
        } else if (card->repetitions == 2) {
            card->interval = 6;
        } else {
            card->interval = (int)round(card->interval * card->ease_factor);
        }
    }
    card->ease_factor = card->ease_factor + (0.1 - (5 - quality) * (0.08 + (5 - quality) * 0.02));
    if (card->ease_factor < MIN_EASE) {
        card->ease_factor = MIN_EASE;
    }
    card->last_reviewed = time(NULL);
    card->due_date = card->last_reviewed + card->interval * 86400;
}

void review_cards(sqlite3 *db) {
    const char *sql =
        "SELECT cards.id, repetitions, ease_factor, interval, last_reviewed, due_date, "
        "(SELECT GROUP_CONCAT(keb, ', ') FROM kanjis WHERE entry_id = cards.entry_id), "
        "(SELECT GROUP_CONCAT(reb, ', ') FROM readings WHERE entry_id = cards.entry_id), "
        "(SELECT GROUP_CONCAT(gloss, ', ') FROM glosses "
        " JOIN senses ON glosses.sense_id = senses.id WHERE senses.entry_id = cards.entry_id) "
        "FROM cards "
        "WHERE due_date <= strftime('%s', 'now') AND status = 'active';";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparando la consulta: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Card c;
        c.id = sqlite3_column_int(stmt, 0);
        c.repetitions = sqlite3_column_int(stmt, 1);
        c.ease_factor = sqlite3_column_double(stmt, 2);
        c.interval = sqlite3_column_int(stmt, 3);
        c.last_reviewed = sqlite3_column_int64(stmt, 4);
        c.due_date = sqlite3_column_int64(stmt, 5);

        const unsigned char *kanji = sqlite3_column_text(stmt, 6);
        const unsigned char *reading = sqlite3_column_text(stmt, 7);
        const unsigned char *gloss = sqlite3_column_text(stmt, 8);

        strncpy(c.kanji, kanji ? (const char *)kanji : "", sizeof(c.kanji));
        strncpy(c.reading, reading ? (const char *)reading : "", sizeof(c.reading));
        strncpy(c.gloss, gloss ? (const char *)gloss : "", sizeof(c.gloss));

        printf("\n🃏 Palabra: %s\nLectura: %s\nSignificado: %s\n", c.kanji, c.reading, c.gloss);
        printf("¿Qué tan bien la recordaste? (0–5): ");
        int q;
        scanf("%d", &q);
        if (q < 0) q = 0; if (q > 5) q = 5;

        update_card(&c, q);

        sqlite3_stmt *upstmt;
        const char *update_sql = "UPDATE cards SET repetitions = ?, ease_factor = ?, interval = ?, "
                                 "last_reviewed = ?, due_date = ? WHERE id = ?;";
        sqlite3_prepare_v2(db, update_sql, -1, &upstmt, NULL);
        sqlite3_bind_int(upstmt, 1, c.repetitions);
        sqlite3_bind_double(upstmt, 2, c.ease_factor);
        sqlite3_bind_int(upstmt, 3, c.interval);
        sqlite3_bind_int64(upstmt, 4, c.last_reviewed);
        sqlite3_bind_int64(upstmt, 5, c.due_date);
        sqlite3_bind_int(upstmt, 6, c.id);
        sqlite3_step(upstmt);
        sqlite3_finalize(upstmt);

        printf("✅ Actualizada. Próxima revisión en %d días.\n", c.interval);
        printf("Presiona ENTER para continuar...\n");
        while (getchar() != '\n'); getchar();
    }

    sqlite3_finalize(stmt);
}
