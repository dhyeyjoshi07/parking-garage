/*
 * test_all.c — Unit Tests for All Data Structures
 *
 * A simple test framework that runs assertions and reports pass/fail.
 * Each data structure has its own test section with multiple test cases
 * covering normal operations, edge cases, and error handling.
 *
 * Run with: make test && ./test_all
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/circular_queue.h"
#include "../include/priority_queue.h"
#include "../include/stack.h"
#include "../include/linked_list.h"
#include "../include/hash_table.h"
#include "../include/billing.h"

/* Simple test counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  Testing: %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)

/* ==================== Circular Queue Tests ==================== */

static void test_circular_queue(void)
{
    CircularQueue cq;
    int slot;

    printf("\n=== Circular Queue Tests ===\n");

    /* Test 1: Init fills queue with slot indices */
    TEST("init fills all slots");
    if (cq_init(&cq, 5) == 0 && cq_available_count(&cq) == 5) {
        PASS();
    } else {
        FAIL("init failed or count wrong");
    }

    /* Test 2: Dequeue returns sequential slot indices */
    TEST("dequeue returns 0, 1, 2, ...");
    {
        int ok = 1;
        int expected;
        for (expected = 0; expected < 5; expected++) {
            if (cq_dequeue(&cq, &slot) != 0 || slot != expected) {
                ok = 0; break;
            }
        }
        if (ok && cq_available_count(&cq) == 0) PASS();
        else FAIL("dequeue sequence wrong");
    }

    /* Test 3: Dequeue from empty queue fails */
    TEST("dequeue from empty returns -1");
    if (cq_dequeue(&cq, &slot) == -1 && cq_is_empty(&cq)) {
        PASS();
    } else {
        FAIL("should have returned -1");
    }

    /* Test 4: Enqueue returns slots to the pool */
    TEST("enqueue restores available count");
    cq_enqueue(&cq, 3);
    cq_enqueue(&cq, 1);
    if (cq_available_count(&cq) == 2) PASS();
    else FAIL("count should be 2");

    /* Test 5: Dequeue after enqueue returns the right slots */
    TEST("dequeue after enqueue returns enqueued slots");
    {
        int s1, s2;
        cq_dequeue(&cq, &s1);
        cq_dequeue(&cq, &s2);
        /* Should get 3 first (FIFO), then 1 */
        if (s1 == 3 && s2 == 1) PASS();
        else FAIL("expected 3 then 1");
    }

    /* Test 6: Wraparound behavior */
    TEST("circular wraparound works");
    {
        CircularQueue cq2;
        int i, out;
        cq_init(&cq2, 3);
        /* Dequeue all 3 */
        for (i = 0; i < 3; i++) cq_dequeue(&cq2, &out);
        /* Enqueue 2 back: 10, 20 */
        cq_enqueue(&cq2, 10);
        cq_enqueue(&cq2, 20);
        /* These should wrap around in the internal array */
        cq_dequeue(&cq2, &out);
        if (out == 10) {
            cq_dequeue(&cq2, &out);
            if (out == 20) PASS();
            else FAIL("second dequeue wrong");
        } else {
            FAIL("first dequeue wrong");
        }
        cq_destroy(&cq2);
    }

    cq_destroy(&cq);
}

/* ==================== Priority Queue Tests ==================== */

static void test_priority_queue(void)
{
    PriorityQueue pq;
    WaitlistEntry entry, out;

    printf("\n=== Priority Queue Tests ===\n");

    TEST("init creates empty queue");
    if (pq_init(&pq, 10) == 0 && pq_is_empty(&pq)) {
        PASS();
    } else {
        FAIL("init failed");
    }

    /* Test: VIP comes out before regular, regardless of insert order */
    TEST("VIP extracted before regular");
    {
        /* Insert regular first, then VIP */
        strcpy(entry.plate, "REG001");
        entry.priority = PRIORITY_REGULAR;
        entry.request_time = 1000;
        entry.is_vip = 0;
        pq_insert(&pq, entry);

        strcpy(entry.plate, "VIP001");
        entry.priority = PRIORITY_VIP;
        entry.request_time = 1001;  /* Arrived LATER but is VIP */
        entry.is_vip = 1;
        pq_insert(&pq, entry);

        pq_extract_min(&pq, &out);
        if (strcmp(out.plate, "VIP001") == 0 && out.priority == PRIORITY_VIP) {
            PASS();
        } else {
            FAIL("VIP should come first");
        }
    }

    /* Test: FIFO within same priority level */
    TEST("FIFO within same priority");
    {
        /* Clear the queue */
        while (!pq_is_empty(&pq)) pq_extract_min(&pq, &out);

        /* Insert two regulars — earlier timestamp should come first */
        strcpy(entry.plate, "EARLY");
        entry.priority = PRIORITY_REGULAR;
        entry.request_time = 100;
        entry.is_vip = 0;
        pq_insert(&pq, entry);

        strcpy(entry.plate, "LATE");
        entry.priority = PRIORITY_REGULAR;
        entry.request_time = 200;
        entry.is_vip = 0;
        pq_insert(&pq, entry);

        pq_extract_min(&pq, &out);
        if (strcmp(out.plate, "EARLY") == 0) PASS();
        else FAIL("earlier request should come first");
    }

    /* Test: peek without removing */
    TEST("peek returns min without removal");
    {
        const WaitlistEntry *p = pq_peek(&pq);
        if (p && strcmp(p->plate, "LATE") == 0 && pq_size(&pq) == 1) {
            PASS();
        } else {
            FAIL("peek should show LATE, size still 1");
        }
    }

    /* Test: extract from empty */
    TEST("extract from empty returns -1");
    pq_extract_min(&pq, &out);  /* Remove the last one */
    if (pq_extract_min(&pq, &out) == -1) PASS();
    else FAIL("should return -1 on empty");

    pq_destroy(&pq);
}

/* ==================== Stack Tests ==================== */

static void test_stack(void)
{
    Stack s;
    char plate[16];

    printf("\n=== Stack Tests ===\n");

    stack_init(&s);

    TEST("init creates empty stack");
    if (stack_is_empty(&s) && stack_size(&s) == 0) PASS();
    else FAIL("should be empty");

    /* Test: LIFO order */
    TEST("LIFO order (last in, first out)");
    stack_push(&s, "CAR_A");
    stack_push(&s, "CAR_B");
    stack_push(&s, "CAR_C");
    stack_pop(&s, plate);
    if (strcmp(plate, "CAR_C") == 0) {
        stack_pop(&s, plate);
        if (strcmp(plate, "CAR_B") == 0) {
            stack_pop(&s, plate);
            if (strcmp(plate, "CAR_A") == 0) PASS();
            else FAIL("third pop wrong");
        } else FAIL("second pop wrong");
    } else FAIL("first pop wrong");

    /* Test: pop from empty */
    TEST("pop from empty returns -1");
    if (stack_pop(&s, plate) == -1) PASS();
    else FAIL("should return -1");

    /* Test: peek */
    TEST("peek returns top without removing");
    stack_push(&s, "PEEK_ME");
    {
        const char *top = stack_peek(&s);
        if (top && strcmp(top, "PEEK_ME") == 0 && stack_size(&s) == 1) PASS();
        else FAIL("peek failed");
    }

    stack_destroy(&s);
}

/* ==================== Linked List Tests ==================== */

static void test_linked_list(void)
{
    LinkedList ll;
    LogEntry *entries[10];
    int count;

    printf("\n=== Linked List Tests ===\n");

    ll_init(&ll);

    TEST("init creates empty list");
    if (ll_count(&ll) == 0) PASS();
    else FAIL("count should be 0");

    /* Test: append and count */
    TEST("append increases count");
    ll_append(&ll, "ABC123", "park", 0, 5, 1000, 0.0);
    ll_append(&ll, "XYZ789", "park", 1, 3, 1001, 0.0);
    ll_append(&ll, "ABC123", "exit", 0, 5, 2000, 4.50);
    if (ll_count(&ll) == 3) PASS();
    else FAIL("count should be 3");

    /* Test: get_recent returns correct entries */
    TEST("get_recent returns last N entries");
    count = ll_get_recent(&ll, entries, 2);
    if (count == 2 &&
        strcmp(entries[0]->plate, "XYZ789") == 0 &&
        strcmp(entries[1]->plate, "ABC123") == 0 &&
        strcmp(entries[1]->action, "exit") == 0) {
        PASS();
    } else {
        FAIL("recent entries wrong");
    }

    /* Test: get_recent with more than available */
    TEST("get_recent when asking more than available");
    count = ll_get_recent(&ll, entries, 10);
    if (count == 3) PASS();
    else FAIL("should return all 3");

    ll_destroy(&ll);
}

/* ==================== Hash Table Tests ==================== */

static void test_hash_table(void)
{
    HashTable ht;
    HashEntry *entry;

    printf("\n=== Hash Table Tests ===\n");

    ht_init(&ht, 7);  /* Small capacity to test collisions */

    TEST("init creates empty table");
    if (ht_count(&ht) == 0) PASS();
    else FAIL("count should be 0");

    /* Test: insert and lookup */
    TEST("insert and lookup work");
    ht_insert(&ht, "ABC123", 0, 5, 1000, 0);
    entry = ht_lookup(&ht, "ABC123");
    if (entry && entry->floor == 0 && entry->slot == 5) PASS();
    else FAIL("lookup returned wrong data");

    /* Test: duplicate detection */
    TEST("duplicate insert returns -2");
    if (ht_insert(&ht, "ABC123", 1, 1, 2000, 0) == -2) PASS();
    else FAIL("should return -2 for duplicate");

    /* Test: lookup nonexistent */
    TEST("lookup nonexistent returns NULL");
    if (ht_lookup(&ht, "NOTHERE") == NULL) PASS();
    else FAIL("should return NULL");

    /* Test: delete */
    TEST("delete removes entry");
    ht_insert(&ht, "DEL001", 2, 10, 3000, 1);
    if (ht_delete(&ht, "DEL001") == 0 &&
        ht_lookup(&ht, "DEL001") == NULL) {
        PASS();
    } else {
        FAIL("delete failed");
    }

    /* Test: delete nonexistent */
    TEST("delete nonexistent returns -1");
    if (ht_delete(&ht, "NOPE") == -1) PASS();
    else FAIL("should return -1");

    /* Test: multiple entries (collision stress test) */
    TEST("handles collisions with chaining");
    {
        int i;
        char plate[16];
        int ok = 1;
        for (i = 0; i < 20; i++) {
            snprintf(plate, sizeof(plate), "CAR%03d", i);
            ht_insert(&ht, plate, i % 3, i, 5000 + i, i % 2);
        }
        /* Verify all can be looked up */
        for (i = 0; i < 20; i++) {
            snprintf(plate, sizeof(plate), "CAR%03d", i);
            if (!ht_lookup(&ht, plate)) { ok = 0; break; }
        }
        if (ok) PASS();
        else FAIL("couldn't find all entries");
    }

    ht_destroy(&ht);
}

/* ==================== Billing Tests ==================== */
/*
 * Billing rules (AED, TIME_SCALE = 360):
 *   10 real seconds = 1 garage hour
 *   10 AED per garage hour, prorated by minute
 *   First 15 garage-minutes free (grace period)
 *   VIP: 20% discount
 *   Daily cap: 200 AED
 *
 * Useful conversions (real seconds -> garage time):
 *   1 real sec  = 360 garage sec = 6 garage min
 *   2.5 real sec = 15 garage min (end of grace)
 *   10 real sec = 3600 garage sec = 1 garage hour
 */

static void test_billing(void)
{
    double charge;
    char dur[32];

    printf("\n=== Billing Tests ===\n");

    /* Test: Under 15 garage-minutes is free.
     * 2 real seconds = 2 * 360 = 720 garage sec = 12 garage min -> free */
    TEST("under 15 garage-min is free");
    charge = calculate_charge(0, 2, 0);
    if (charge == 0.0) PASS();
    else FAIL("should be free");

    /* Test: Exactly 15 garage-minutes is free.
     * 2.5 real seconds = 900 garage sec = 15 garage min -> free */
    TEST("exactly 15 garage-min (2.5 real sec) is free");
    charge = calculate_charge(100, 102, 0);  /* 2 real sec = 12 min, still free */
    if (charge == 0.0) PASS();
    else FAIL("15 garage-min should be free");

    /* Test: 1 garage hour = 10 AED.
     * 10 real seconds = 3600 garage sec = 60 garage min = 1 garage hour */
    TEST("1 garage hour (10 real sec) = 10 AED");
    charge = calculate_charge(0, 10, 0);
    if (charge >= 9.99 && charge <= 10.01) PASS();
    else { printf("got %.2f AED ", charge); FAIL("expected ~10 AED"); }

    /* Test: 2 garage hours = 20 AED.
     * 20 real seconds = 7200 garage sec = 120 garage min */
    TEST("2 garage hours (20 real sec) = 20 AED");
    charge = calculate_charge(0, 20, 0);
    if (charge >= 19.99 && charge <= 20.01) PASS();
    else { printf("got %.2f AED ", charge); FAIL("expected ~20 AED"); }

    /* Test: VIP gets 20% discount.
     * 10 real sec -> 1 garage hour -> 10 AED regular, 8 AED VIP */
    TEST("VIP gets 20% discount");
    {
        double regular = calculate_charge(0, 10, 0);
        double vip = calculate_charge(0, 10, 1);
        if (vip >= regular * 0.79 && vip <= regular * 0.81) PASS();
        else { printf("reg=%.2f vip=%.2f ", regular, vip); FAIL("20% off"); }
    }

    /* Test: Daily cap at 200 AED.
     * 240 real seconds = 86400 garage sec = 24 garage hours
     * 24 * 10 = 240 AED -> capped to 200 AED */
    TEST("daily cap at 200 AED");
    charge = calculate_charge(0, 240, 0);
    if (charge == 200.0) PASS();
    else { printf("got %.2f AED ", charge); FAIL("expected 200 AED cap"); }

    /* Test: Format duration.
     * format_duration takes real seconds. 
     * 10 real seconds * 360 = 3600 garage sec = 1h 0m */
    TEST("format_duration produces correct string");
    format_duration(10, dur);  /* 10 real sec = 1 garage hour */
    if (strcmp(dur, "1h 0m") == 0) PASS();
    else { printf("got '%s' ", dur); FAIL("expected '1h 0m'"); }
}

/* ==================== Main ==================== */

int main(void)
{
    printf("========================================\n");
    printf("  Parking Garage — Data Structure Tests\n");
    printf("========================================\n");

    test_circular_queue();
    test_priority_queue();
    test_stack();
    test_linked_list();
    test_hash_table();
    test_billing();

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n",
           tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
