/*
 * garage.c — Core Parking Garage Manager (Orchestration Layer)
 *
 * See garage.h for design rationale and API documentation.
 *
 * This file ties together all five data structures:
 *   - CircularQueue (per floor) for slot allocation
 *   - PriorityQueue for the VIP-priority waitlist
 *   - HashTable for O(1) plate-to-location lookup
 *   - LinkedList for the chronological activity log
 *   - Stack for the exit-lane LIFO simulation
 *
 * STATE PERSISTENCE:
 *   Since the C program runs as a CLI tool called by Flask (one invocation
 *   per request), all state must be saved to disk after every operation
 *   and reloaded at the start of the next. We use a simple binary format:
 *
 *   1. Write the magic number (GARAGE_MAGIC) for validation
 *   2. Write each floor's circular queue data
 *   3. Write the hash table entries (serialized as flat records)
 *   4. Write the waitlist entries
 *   5. Write the activity log entries
 *   6. Write the exit lane stack entries
 *
 *   This is much simpler than a full database and sufficient for a
 *   single-user local application.
 */

#include "../include/garage.h"
#include "../include/billing.h"
#include <stdio.h>    /* FILE, fopen, fwrite, fread, fclose */
#include <stdlib.h>   /* malloc, free */
#include <string.h>   /* strncpy, strcmp, memset, snprintf */
#include <time.h>     /* time() */

/* ==================== Initialization ==================== */

int garage_init(Garage *g)
{
    int i;

    if (!g) return -1;

    /* Initialize each floor's circular queue with SLOTS_PER_FLOOR slots */
    for (i = 0; i < NUM_FLOORS; i++) {
        if (cq_init(&g->floors[i], SLOTS_PER_FLOOR) != 0) {
            return -1;
        }
    }

    /* Initialize the waitlist (capacity = MAX_WAITLIST) */
    if (pq_init(&g->waitlist, MAX_WAITLIST) != 0) {
        return -1;
    }

    /* Initialize the hash table (101 buckets — prime for distribution) */
    if (ht_init(&g->vehicles, HASH_BUCKETS) != 0) {
        return -1;
    }

    /* Initialize the activity log and exit lane */
    ll_init(&g->log);
    stack_init(&g->exit_lane);

    g->total_revenue_cents = 0;
    g->initialized = GARAGE_MAGIC;

    return 0;
}

/* ==================== Park Operation ==================== */

/*
 * garage_park — Try to park a vehicle, or add to waitlist if full.
 *
 * Algorithm:
 *   1. Check the hash table for duplicates (vehicle already parked).
 *   2. Iterate floors 0, 1, 2 — try dequeuing a slot from each
 *      floor's circular queue. First success wins (nearest floor).
 *   3. If no floor has space, add to the priority waitlist.
 *   4. On successful park: insert into hash table + log the event.
 */
void garage_park(Garage *g, const char *plate, int is_vip, ParkResult *result)
{
    int i, slot;
    long now;

    if (!g || !plate || !result) return;

    memset(result, 0, sizeof(ParkResult));

    /* Step 1: Check for duplicate plates */
    if (ht_lookup(&g->vehicles, plate) != NULL) {
        result->result_code = GARAGE_ERR_DUPLICATE;
        result->floor = -1;
        result->slot = -1;
        result->waitlist_pos = -1;
        snprintf(result->message, sizeof(result->message),
                 "Vehicle %s is already parked", plate);
        return;
    }

    now = time(NULL);

    /* Step 2: Try each floor for an available slot */
    for (i = 0; i < NUM_FLOORS; i++) {
        if (cq_dequeue(&g->floors[i], &slot) == 0) {
            /*
             * Found a slot! Insert into hash table for O(1) lookup,
             * and log the park event.
             */
            ht_insert(&g->vehicles, plate, i, slot, now, is_vip);
            ll_append(&g->log, plate, "park", i, slot, now, 0.0);

            result->result_code = GARAGE_OK;
            result->floor = i;
            result->slot = slot;
            result->waitlist_pos = -1;
            snprintf(result->message, sizeof(result->message),
                     "Parked on floor %d, slot %d", i + 1, slot + 1);
            return;
        }
    }

    /* Step 3: All floors full — add to priority waitlist */
    {
        WaitlistEntry entry;
        strncpy(entry.plate, plate, 15);
        entry.plate[15] = '\0';
        entry.is_vip = is_vip;
        entry.request_time = now;

        /* Set priority level based on VIP status */
        if (is_vip) {
            entry.priority = PRIORITY_VIP;
        } else {
            entry.priority = PRIORITY_REGULAR;
        }

        if (pq_insert(&g->waitlist, entry) == 0) {
            result->result_code = GARAGE_ERR_WAITLIST;
            result->floor = -1;
            result->slot = -1;
            result->waitlist_pos = pq_size(&g->waitlist);
            snprintf(result->message, sizeof(result->message),
                     "Garage full — added to waitlist (position %d)",
                     result->waitlist_pos);
        } else {
            result->result_code = GARAGE_ERR_FULL;
            result->floor = -1;
            result->slot = -1;
            result->waitlist_pos = -1;
            snprintf(result->message, sizeof(result->message),
                     "Garage full and waitlist is full");
        }
    }
}

/* ==================== Exit Operation ==================== */

/*
 * garage_exit — Remove a vehicle and compute billing.
 *
 * Algorithm:
 *   1. Look up the vehicle in the hash table.
 *   2. Calculate the charge based on parking duration.
 *   3. Return the slot to the floor's circular queue.
 *   4. Remove from hash table, log the exit event.
 *   5. If waitlist is not empty, automatically park the highest-
 *      priority waiting vehicle in the newly freed slot.
 */
void garage_exit(Garage *g, const char *plate, ExitResult *result)
{
    HashEntry *entry;
    long now;
    int floor, slot, is_vip;
    long entry_time;
    double charge;

    if (!g || !plate || !result) return;

    memset(result, 0, sizeof(ExitResult));
    now = time(NULL);

    /* Step 1: Find the vehicle */
    entry = ht_lookup(&g->vehicles, plate);
    if (!entry) {
        result->result_code = GARAGE_ERR_NOT_FOUND;
        snprintf(result->message, sizeof(result->message),
                 "Vehicle %s not found in garage", plate);
        return;
    }

    /* Save entry data before we delete it */
    floor = entry->floor;
    slot = entry->slot;
    entry_time = entry->entry_time;
    is_vip = entry->is_vip;
    strncpy(result->plate, plate, 15);
    result->plate[15] = '\0';

    /* Step 2: Calculate billing */
    charge = calculate_charge(entry_time, now, is_vip);

    /* Step 3: Return the slot to the floor's circular queue */
    cq_enqueue(&g->floors[floor], slot);

    /* Step 4: Remove from hash table and log the exit */
    ht_delete(&g->vehicles, plate);
    ll_append(&g->log, plate, "exit", floor, slot, now, charge);

    /* Populate the result */
    result->result_code = GARAGE_OK;
    result->floor = floor;
    result->slot = slot;
    result->entry_time = entry_time;
    result->exit_time = now;
    result->duration_seconds = now - entry_time;
    result->amount = charge;
    result->is_vip = is_vip;
    format_duration(result->duration_seconds, result->duration_str);

    /* Track revenue (in cents to avoid floating point accumulation errors) */
    g->total_revenue_cents += (int)(charge * 100);

    snprintf(result->message, sizeof(result->message),
             "Vehicle %s exited — charged $%.2f (%s)",
             plate, charge, result->duration_str);

    /*
     * Step 5: If vehicles are waiting, admit the highest-priority one
     * into the slot that just freed up.
     *
     * This is where the priority queue shines — extract_min gives us
     * the VIP vehicle first, even if regular vehicles arrived earlier.
     */
    {
        WaitlistEntry waitlist_entry;
        if (pq_extract_min(&g->waitlist, &waitlist_entry) == 0) {
            /* Park the waiting vehicle in the freed slot */
            ht_insert(&g->vehicles, waitlist_entry.plate,
                      floor, slot, now, waitlist_entry.is_vip);

            /*
             * Re-dequeue the slot we just enqueued (it was just freed,
             * now it's being reassigned to the waitlisted vehicle).
             */
            {
                int reassigned_slot;
                cq_dequeue(&g->floors[floor], &reassigned_slot);
            }

            ll_append(&g->log, waitlist_entry.plate, "park",
                      floor, slot, now, 0.0);
        }
    }
}

/* ==================== Lookup Operation ==================== */

void garage_lookup(Garage *g, const char *plate, LookupResult *result)
{
    HashEntry *entry;

    if (!g || !plate || !result) return;

    memset(result, 0, sizeof(LookupResult));

    entry = ht_lookup(&g->vehicles, plate);
    if (!entry) {
        result->result_code = GARAGE_ERR_NOT_FOUND;
        snprintf(result->message, sizeof(result->message),
                 "Vehicle %s not found", plate);
        return;
    }

    result->result_code = GARAGE_OK;
    strncpy(result->plate, entry->plate, 15);
    result->plate[15] = '\0';
    result->floor = entry->floor;
    result->slot = entry->slot;
    result->entry_time = entry->entry_time;
    result->is_vip = entry->is_vip;
    snprintf(result->message, sizeof(result->message),
             "Found on floor %d, slot %d", entry->floor + 1, entry->slot + 1);
}

/* ==================== Cleanup ==================== */

void garage_destroy(Garage *g)
{
    int i;
    if (!g) return;

    for (i = 0; i < NUM_FLOORS; i++) {
        cq_destroy(&g->floors[i]);
    }
    pq_destroy(&g->waitlist);
    ht_destroy(&g->vehicles);
    ll_destroy(&g->log);
    stack_destroy(&g->exit_lane);
}

/* ==================== State Persistence ==================== */

/*
 * BINARY FILE FORMAT:
 *
 *   [4 bytes]  Magic number (GARAGE_MAGIC)
 *   [4 bytes]  Total revenue in cents
 *
 *   For each floor (NUM_FLOORS times):
 *     [4 bytes]  Circular queue count
 *     [4 bytes]  Circular queue front index
 *     [4 bytes]  Circular queue rear index
 *     [N * 4 bytes]  Queue data (N = SLOTS_PER_FLOOR)
 *
 *   [4 bytes]  Number of parked vehicles (hash table entries)
 *   For each parked vehicle:
 *     [16 bytes]  plate
 *     [4 bytes]   floor
 *     [4 bytes]   slot
 *     [8 bytes]   entry_time (long)
 *     [4 bytes]   is_vip
 *
 *   [4 bytes]  Number of waitlist entries
 *   For each waitlist entry:
 *     [16 bytes]  plate
 *     [4 bytes]   priority
 *     [8 bytes]   request_time
 *     [4 bytes]   is_vip
 *
 *   [4 bytes]  Number of log entries
 *   For each log entry:
 *     [16 bytes]  plate
 *     [8 bytes]   action
 *     [4 bytes]   floor
 *     [4 bytes]   slot
 *     [8 bytes]   timestamp
 *     [8 bytes]   amount (double)
 *
 *   [4 bytes]  Number of exit-lane entries
 *   For each exit-lane entry:
 *     [16 bytes]  plate
 */

int garage_save(const Garage *g, const char *filepath)
{
    FILE *f;
    int i, count;

    if (!g || !filepath) return -1;

    f = fopen(filepath, "wb");
    if (!f) return -1;

    /* Write magic number and revenue */
    fwrite(&g->initialized, sizeof(int), 1, f);
    fwrite(&g->total_revenue_cents, sizeof(int), 1, f);

    /* Write each floor's circular queue state */
    for (i = 0; i < NUM_FLOORS; i++) {
        fwrite(&g->floors[i].count, sizeof(int), 1, f);
        fwrite(&g->floors[i].front, sizeof(int), 1, f);
        fwrite(&g->floors[i].rear, sizeof(int), 1, f);
        fwrite(g->floors[i].data, sizeof(int), SLOTS_PER_FLOOR, f);
    }

    /* Write parked vehicles from hash table */
    count = ht_count(&g->vehicles);
    fwrite(&count, sizeof(int), 1, f);
    for (i = 0; i < g->vehicles.capacity; i++) {
        HashEntry *entry = g->vehicles.buckets[i];
        while (entry) {
            fwrite(entry->plate, sizeof(char), 16, f);
            fwrite(&entry->floor, sizeof(int), 1, f);
            fwrite(&entry->slot, sizeof(int), 1, f);
            fwrite(&entry->entry_time, sizeof(long), 1, f);
            fwrite(&entry->is_vip, sizeof(int), 1, f);
            entry = entry->next;
        }
    }

    /* Write waitlist entries (directly from the heap array) */
    fwrite(&g->waitlist.size, sizeof(int), 1, f);
    for (i = 0; i < g->waitlist.size; i++) {
        fwrite(g->waitlist.heap[i].plate, sizeof(char), 16, f);
        fwrite(&g->waitlist.heap[i].priority, sizeof(int), 1, f);
        fwrite(&g->waitlist.heap[i].request_time, sizeof(long), 1, f);
        fwrite(&g->waitlist.heap[i].is_vip, sizeof(int), 1, f);
    }

    /* Write activity log (walk the linked list) */
    count = ll_count(&g->log);
    fwrite(&count, sizeof(int), 1, f);
    {
        LogEntry *entry = g->log.head;
        while (entry) {
            fwrite(entry->plate, sizeof(char), 16, f);
            fwrite(entry->action, sizeof(char), 8, f);
            fwrite(&entry->floor, sizeof(int), 1, f);
            fwrite(&entry->slot, sizeof(int), 1, f);
            fwrite(&entry->timestamp, sizeof(long), 1, f);
            fwrite(&entry->amount, sizeof(double), 1, f);
            entry = entry->next;
        }
    }

    /* Write exit lane stack (walk from top to bottom) */
    {
        StackNode *node = g->exit_lane.top;
        count = stack_size(&g->exit_lane);
        fwrite(&count, sizeof(int), 1, f);
        while (node) {
            fwrite(node->plate, sizeof(char), 16, f);
            node = node->next;
        }
    }

    fclose(f);
    return 0;
}

/*
 * garage_load — Reconstruct the entire garage state from a binary file.
 *
 * We read each section in the same order it was written by garage_save.
 * If the file doesn't exist or the magic number doesn't match, we
 * fall back to garage_init (fresh start).
 */
int garage_load(Garage *g, const char *filepath)
{
    FILE *f;
    int i, count, magic;

    if (!g || !filepath) return -1;

    f = fopen(filepath, "rb");
    if (!f) return -1;  /* File doesn't exist — caller should init fresh */

    /* Read and validate magic number */
    fread(&magic, sizeof(int), 1, f);
    if (magic != GARAGE_MAGIC) {
        fclose(f);
        return -1;  /* Corrupt or wrong file */
    }

    /* Read revenue */
    fread(&g->total_revenue_cents, sizeof(int), 1, f);
    g->initialized = GARAGE_MAGIC;

    /* Read each floor's circular queue state */
    for (i = 0; i < NUM_FLOORS; i++) {
        /* Allocate the queue's internal array */
        g->floors[i].data = (int *)malloc(sizeof(int) * SLOTS_PER_FLOOR);
        if (!g->floors[i].data) { fclose(f); return -1; }
        g->floors[i].capacity = SLOTS_PER_FLOOR;

        fread(&g->floors[i].count, sizeof(int), 1, f);
        fread(&g->floors[i].front, sizeof(int), 1, f);
        fread(&g->floors[i].rear, sizeof(int), 1, f);
        fread(g->floors[i].data, sizeof(int), SLOTS_PER_FLOOR, f);
    }

    /* Read parked vehicles into hash table */
    if (ht_init(&g->vehicles, HASH_BUCKETS) != 0) {
        fclose(f); return -1;
    }
    fread(&count, sizeof(int), 1, f);
    for (i = 0; i < count; i++) {
        char plate[16];
        int fl, sl, vip;
        long etime;
        fread(plate, sizeof(char), 16, f);
        fread(&fl, sizeof(int), 1, f);
        fread(&sl, sizeof(int), 1, f);
        fread(&etime, sizeof(long), 1, f);
        fread(&vip, sizeof(int), 1, f);
        ht_insert(&g->vehicles, plate, fl, sl, etime, vip);
    }

    /* Read waitlist entries into priority queue */
    if (pq_init(&g->waitlist, MAX_WAITLIST) != 0) {
        fclose(f); return -1;
    }
    fread(&count, sizeof(int), 1, f);
    for (i = 0; i < count; i++) {
        WaitlistEntry entry;
        fread(entry.plate, sizeof(char), 16, f);
        fread(&entry.priority, sizeof(int), 1, f);
        fread(&entry.request_time, sizeof(long), 1, f);
        fread(&entry.is_vip, sizeof(int), 1, f);
        pq_insert(&g->waitlist, entry);
    }

    /* Read activity log into linked list */
    ll_init(&g->log);
    fread(&count, sizeof(int), 1, f);
    for (i = 0; i < count; i++) {
        char plate[16], action[8];
        int fl, sl;
        long ts;
        double amt;
        fread(plate, sizeof(char), 16, f);
        fread(action, sizeof(char), 8, f);
        fread(&fl, sizeof(int), 1, f);
        fread(&sl, sizeof(int), 1, f);
        fread(&ts, sizeof(long), 1, f);
        fread(&amt, sizeof(double), 1, f);
        ll_append(&g->log, plate, action, fl, sl, ts, amt);
    }

    /* Read exit lane stack entries */
    stack_init(&g->exit_lane);
    fread(&count, sizeof(int), 1, f);
    /*
     * Stack complication: we saved top-to-bottom, but pushing restores
     * them in reverse order. Read all plates first, then push in
     * reverse to restore the original order.
     */
    if (count > 0) {
        char plates[100][16]; /* Max 100 cars in exit lane */
        int actual = (count < 100) ? count : 100;
        for (i = 0; i < actual; i++) {
            fread(plates[i], sizeof(char), 16, f);
        }
        /* Push in reverse order so the first-saved becomes top again */
        for (i = actual - 1; i >= 0; i--) {
            stack_push(&g->exit_lane, plates[i]);
        }
    }

    fclose(f);
    return 0;
}
