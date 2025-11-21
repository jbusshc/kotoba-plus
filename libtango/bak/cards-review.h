#ifndef CARDS_REVIEW_H
#define CARDS_REVIEW_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <math.h>

typedef struct {
    int id;
    int repetitions;
    double ease_factor;
    int interval;
    time_t last_reviewed;
    time_t due_date;
    char kanji[256];
    char reading[256];
    char gloss[512];
} Card;

#define MIN_EASE 1.3

#ifdef __cplusplus
extern "C" {
#endif

void update_card(Card* card, int quality);
void review_cards(sqlite3 *db);

#ifdef __cplusplus
}
#endif

#endif // CARDS_REVIEW_H
