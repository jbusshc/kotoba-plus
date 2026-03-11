/*
 * fsrs_policy_test.c — verifica el comportamiento de scheduling tipo Anki
 *
 * Compila: gcc -O2 -std=c11 fsrs.c fsrs_policy_test.c -lm -o fsrs_policy_test
 */

#include "fsrs.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define PASS "\033[32mPASS\033[0m"
#define FAIL "\033[31mFAIL\033[0m"
static int _ok = 0, _fail = 0;
#define CHECK(expr) do { \
    if (expr) { printf("  " PASS "  %s\n", #expr); _ok++; } \
    else      { printf("  " FAIL "  %s  (line %d)\n", #expr, __LINE__); _fail++; } \
} while(0)

/* Simulated day start: 04:00 UTC */
#define DAY_OFF  (4 * 3600ULL)
#define DAY_SEC  86400ULL

/* ── helpers ── */
static uint64_t day_start(uint64_t day) { return day * DAY_SEC + DAY_OFF; }

/* ─────────────────────────────────────────────────── */
static void test_snap_to_day(void) {
    printf("\n[Review due snaps to start of logical day]\n");

    fsrs_deck d;
    fsrs_deck_init(&d, 16, NULL);

    /* Answer at 14:00 on day 0 */
    uint64_t t0  = day_start(0) + 10 * 3600; /* 14:00 */
    fsrs_add_card(&d, 1, t0);
    fsrs_card *c = fsrs_next_card(&d, t0);
    fsrs_answer(&d, c, FSRS_EASY, t0); /* graduates with N days interval */
    c = fsrs_get_card(&d, 1);

    /* due must be exactly at a day boundary (== DAY_OFF mod DAY_SEC) */
    uint64_t due_offset = (c->due - DAY_OFF) % DAY_SEC;
    printf("  due=%llu  offset_within_day=%llu  (expected 0)\n",
           (unsigned long long)c->due, (unsigned long long)due_offset);
    CHECK(due_offset == 0);

    /* due must be in the future (at least tomorrow's boundary) */
    CHECK(c->due > t0);

    fsrs_deck_free(&d);
}

/* ─────────────────────────────────────────────────── */
static void test_learning_intercalado(void) {
    printf("\n[Learning cards intercaladas con reviews]\n");
    /*
     * Scenario:
     *   card 1 → AGAIN (learning step: 1 min) → due in 60s
     *   card 2 → AGAIN (learning step: 1 min) → due in 60s
     *   card 3 → review due now
     *
     * Before 60s pass: next_card should return card 3 (the review).
     * After 60s:       next_card should return card 1 or 2 (learning).
     */

    /* custom steps: 60s only */
    uint32_t steps[] = {60};
    fsrs_params p = fsrs_default_params();
    p.learning_steps       = steps;
    p.learning_steps_count = 1;
    p.new_cards_per_day    = 100;

    fsrs_deck d;
    fsrs_deck_init(&d, 16, &p);

    uint64_t now = day_start(10);

    /* add 3 new cards */
    for (uint32_t i = 1; i <= 3; i++) fsrs_add_card(&d, i, now);

    /* cards 1 & 2: answer AGAIN → go to learning step 0 (due +60s) */
    for (int i = 0; i < 2; i++) {
        fsrs_card *c = fsrs_next_card(&d, now);
        assert(c && c->state == FSRS_STATE_NEW);
        fsrs_answer(&d, c, FSRS_AGAIN, now);
    }
    /* card 3: answer EASY → graduates → due at start of tomorrow */
    {
        fsrs_card *c = fsrs_next_card(&d, now);
        assert(c && c->state == FSRS_STATE_NEW);
        uint32_t id3 = c->id;
        fsrs_answer(&d, c, FSRS_EASY, now);
        (void)id3;
    }

    /* --- now is still t=0, cards 1&2 due in 60s, card 3 due tomorrow --- */

    /* next card should be nothing immediately available (learning not yet due,
     * reviews due tomorrow). But because there are no more reviews/new today,
     * Anki shows the earliest learning card immediately. */
    fsrs_card *nxt = fsrs_next_card(&d, now);
    printf("  next (t=0, no reviews left): id=%u state=%d\n",
           nxt ? nxt->id : 0, nxt ? nxt->state : -1);
    CHECK(nxt != NULL);
    CHECK(nxt->state == FSRS_STATE_LEARNING);  /* fallback: show learning even if timer not up */

    fsrs_deck_free(&d);
}

/* ─────────────────────────────────────────────────── */
static void test_learning_waits_when_reviews_exist(void) {
    printf("\n[Learning card espera cuando hay reviews disponibles]\n");
    /*
     * card 1 → AGAIN → learning, due +60s
     * card 2 → graduated (simulate), due now as a review
     *
     * next_card at t=0 should return card 2 (review), not card 1 (learning timer not up).
     * next_card at t=65 should return card 1 (learning timer up, card 2 already done).
     */

    uint32_t steps[] = {60};
    fsrs_params p = fsrs_default_params();
    p.learning_steps       = steps;
    p.learning_steps_count = 1;
    p.new_cards_per_day    = 100;
    p.reviews_per_day      = 0; /* unlimited */

    fsrs_deck d;
    fsrs_deck_init(&d, 16, &p);

    uint64_t now = day_start(10);

    fsrs_add_card(&d, 1, now);
    fsrs_add_card(&d, 2, now);

    /* card 1: AGAIN → learning */
    fsrs_card *c1 = fsrs_next_card(&d, now);
    assert(c1->id == 1 || c1->id == 2);
    uint32_t id1 = c1->id;
    fsrs_answer(&d, c1, FSRS_AGAIN, now);

    /* card 2: EASY → review, due tomorrow */
    fsrs_card *c2 = fsrs_next_card(&d, now);
    uint32_t id2 = c2->id;
    fsrs_answer(&d, c2, FSRS_EASY, now);
    /* force card 2 due to NOW so it shows as a review today */
    fsrs_card *fc2 = fsrs_get_card(&d, id2);
    fc2->due = now; /* override snap for test purposes */
    /* rebuild heap so the changed due is respected */
    fsrs_deck_rebuild_queues(&d, now);

    /* at t=0: learning card timer not up, but review exists → review first */
    fsrs_card *nxt = fsrs_next_card(&d, now);
    printf("  next at t=0: id=%u state=%d  (want review id=%u)\n",
           nxt ? nxt->id : 0, nxt ? nxt->state : -1, id2);
    CHECK(nxt != NULL);
    CHECK(nxt->id == id2);
    CHECK(nxt->state == FSRS_STATE_REVIEW);

    /* answer that review */
    fsrs_answer(&d, nxt, FSRS_GOOD, now);

    /* at t=65: learning timer expired → learning card next */
    uint64_t t65 = now + 65;
    fsrs_card *nxt2 = fsrs_next_card(&d, t65);
    printf("  next at t=65: id=%u state=%d  (want learning id=%u)\n",
           nxt2 ? nxt2->id : 0, nxt2 ? nxt2->state : -1, id1);
    CHECK(nxt2 != NULL);
    CHECK(nxt2->id == id1);
    CHECK(nxt2->state == FSRS_STATE_LEARNING);

    fsrs_deck_free(&d);
}

/* ─────────────────────────────────────────────────── */
static void test_only_learning_cards_no_wait(void) {
    printf("\n[Solo 2 AGAIN cards — no hay más nada → se muestran sin esperar]\n");

    uint32_t steps[] = {60, 600};
    fsrs_params p = fsrs_default_params();
    p.learning_steps       = steps;
    p.learning_steps_count = 2;
    p.new_cards_per_day    = 2;

    fsrs_deck d;
    fsrs_deck_init(&d, 16, &p);
    uint64_t now = day_start(5);

    fsrs_add_card(&d, 1, now);
    fsrs_add_card(&d, 2, now);

    /* Answer both AGAIN */
    for (int i = 0; i < 2; i++) {
        fsrs_card *c = fsrs_next_card(&d, now);
        assert(c && c->state == FSRS_STATE_NEW);
        fsrs_answer(&d, c, FSRS_AGAIN, now);
    }

    /* Now: both cards in learning, due in 60s. No reviews, no new left.
     * fsrs_next_card should return one of them immediately (no NULL). */
    fsrs_card *nxt = fsrs_next_card(&d, now);
    printf("  next at t=0 (timer not up, nothing else): %s\n",
           nxt ? "card available" : "NULL");
    CHECK(nxt != NULL);
    CHECK(nxt->state == FSRS_STATE_LEARNING);

    /* fsrs_wait_seconds should return 0 because we serve it immediately */
    uint64_t wait = fsrs_wait_seconds(&d, now);
    printf("  fsrs_wait_seconds = %llu  (expected 0)\n", (unsigned long long)wait);
    CHECK(wait == 0);

    fsrs_deck_free(&d);
}

/* ─────────────────────────────────────────────────── */
static void test_wait_seconds_with_pending_learning(void) {
    printf("\n[fsrs_wait_seconds cuando hay reviews Y learning pendiente]\n");

    uint32_t steps[] = {120}; /* 2 min step */
    fsrs_params p = fsrs_default_params();
    p.learning_steps       = steps;
    p.learning_steps_count = 1;
    p.new_cards_per_day    = 1;
    p.reviews_per_day      = 0;

    fsrs_deck d;
    fsrs_deck_init(&d, 16, &p);
    uint64_t now = day_start(3);

    fsrs_add_card(&d, 1, now);

    /* AGAIN → learning, due +120s */
    fsrs_card *c = fsrs_next_card(&d, now);
    fsrs_answer(&d, c, FSRS_AGAIN, now);

    /* nothing else in queue → immediately served → wait = 0 */
    uint64_t w0 = fsrs_wait_seconds(&d, now);
    printf("  wait with only learning card (no other queue): %llu  (expected 0)\n",
           (unsigned long long)w0);
    CHECK(w0 == 0);

    /* now add a second card that we graduate, so there IS a review due +1s */
    fsrs_add_card(&d, 2, now);
    fsrs_card *c2 = fsrs_next_card(&d, now);
    fsrs_answer(&d, c2, FSRS_EASY, now);
    /* force it due in 1 second */
    fsrs_get_card(&d, 2)->due = now + 1;
    fsrs_deck_rebuild_queues(&d, now);

    /* now: review due in 1s, learning due in 120s.
     * next_card gives the review (not yet due, but within today).
     * Actually review due > now, so next_card gives learning fallback... 
     * wait = 1 (seconds until review) */
    fsrs_card *nxt = fsrs_next_card(&d, now);
    printf("  next at t=0 with review+1s: id=%u state=%d\n",
           nxt ? nxt->id : 0, nxt ? nxt->state : -1);
    /* review is due at now+1, not yet; learning fallback shows immediately */
    /* This is correct: learning card shows up since nothing CURRENTLY due */

    /* at t=now+1, the review should come first */
    fsrs_card *nxt1 = fsrs_next_card(&d, now + 1);
    printf("  next at t+1: id=%u state=%d  (want review)\n",
           nxt1 ? nxt1->id : 0, nxt1 ? nxt1->state : -1);
    CHECK(nxt1 != NULL);
    CHECK(nxt1->state == FSRS_STATE_REVIEW);

    fsrs_deck_free(&d);
}

/* ─────────────────────────────────────────────────── */
static void test_reviews_only_shown_today(void) {
    printf("\n[Reviews due mañana NO aparecen hoy]\n");

    fsrs_params p = fsrs_default_params();
    p.new_cards_per_day = 1;

    fsrs_deck d;
    fsrs_deck_init(&d, 16, &p);
    uint64_t now = day_start(0);

    fsrs_add_card(&d, 1, now);
    fsrs_card *c = fsrs_next_card(&d, now);
    fsrs_answer(&d, c, FSRS_EASY, now); /* graduates, snaps to tomorrow+ */

    c = fsrs_get_card(&d, 1);
    printf("  card due: day %llu  (today day 0)\n",
           (unsigned long long)((c->due - DAY_OFF) / DAY_SEC));
    CHECK(c->due >= day_start(1)); /* due at least tomorrow */

    /* today: nothing to show */
    fsrs_card *nxt = fsrs_next_card(&d, now);
    printf("  next_card today: %s  (expected NULL)\n", nxt ? "card" : "NULL");
    CHECK(nxt == NULL);

    /* tomorrow: card appears */
    uint64_t tomorrow = c->due + 1; /* just after due */
    fsrs_card *nxt2 = fsrs_next_card(&d, tomorrow);
    printf("  next_card tomorrow+1s: %s  (expected card)\n", nxt2 ? "card" : "NULL");
    CHECK(nxt2 != NULL);

    fsrs_deck_free(&d);
}

/* ─────────────────────────────────────────────────── */
int main(void) {
    printf("══════════════════════════════════════════\n");
    printf("  FSRS-5 — Scheduling Policy Tests\n");
    printf("══════════════════════════════════════════\n");

    test_snap_to_day();
    test_only_learning_cards_no_wait();
    test_learning_intercalado();
    test_learning_waits_when_reviews_exist();
    test_wait_seconds_with_pending_learning();
    test_reviews_only_shown_today();

    printf("\n══════════════════════════════════════════\n");
    printf("  Results: %d passed, %d failed\n", _ok, _fail);
    printf("══════════════════════════════════════════\n");
    return _fail ? 1 : 0;
}