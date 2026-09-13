/*
 * hash_table.c — Hash Table for License Plate → Slot Lookup
 *
 * See hash_table.h for design rationale and API documentation.
 *
 * HASH FUNCTION — DJB2:
 *   hash = 5381
 *   for each character c:  hash = hash * 33 + c
 *
 *   This is Dan Bernstein's classic hash function. The magic number 5381
 *   and multiplier 33 were empirically chosen for good distribution on
 *   short ASCII strings — ideal for license plates like "ABC1234".
 *
 * COLLISION RESOLUTION — Separate Chaining:
 *   Each bucket holds a linked list of entries. On collision, the new
 *   entry is prepended to the list (O(1) insertion). Lookups walk the
 *   chain comparing plates (O(k) where k = chain length, typically 1-2).
 */

#include "../include/hash_table.h"
#include <stdlib.h>   /* malloc, free */
#include <string.h>   /* strcmp, strncpy */

/*
 * djb2_hash — Compute the DJB2 hash of a string.
 *
 * We return an unsigned long to avoid negative values when taking
 * modulo with the bucket count. The cast to unsigned char ensures
 * characters with high bits set don't produce negative intermediate values.
 */
static unsigned long djb2_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = (unsigned char)*str++)) {
        /*
         * hash * 33 + c
         * The bit-shift version (hash << 5) + hash is equivalent to
         * hash * 32 + hash = hash * 33, but avoids multiplication.
         */
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

/* ==================== Public API ==================== */

int ht_init(HashTable *ht, int capacity)
{
    int i;

    if (!ht || capacity <= 0) return -1;

    ht->buckets = (HashEntry **)malloc(sizeof(HashEntry *) * capacity);
    if (!ht->buckets) return -1;

    /* Initialize all bucket heads to NULL (empty chains) */
    for (i = 0; i < capacity; i++) {
        ht->buckets[i] = NULL;
    }

    ht->capacity = capacity;
    ht->count = 0;
    return 0;
}

/*
 * ht_insert — Hash the plate, check for duplicates, prepend to chain.
 *
 * Steps:
 *   1. Compute bucket index = djb2_hash(plate) % capacity
 *   2. Walk the chain at that bucket to check for duplicate plates
 *   3. If no duplicate, allocate a new entry and prepend to the chain
 */
int ht_insert(HashTable *ht, const char *plate, int floor, int slot,
              long entry_time, int is_vip)
{
    unsigned long hash;
    int bucket;
    HashEntry *entry, *current;

    if (!ht || !plate) return -1;

    hash = djb2_hash(plate);
    bucket = (int)(hash % (unsigned long)ht->capacity);

    /* Check for duplicate — walk the chain at this bucket */
    current = ht->buckets[bucket];
    while (current) {
        if (strcmp(current->plate, plate) == 0) {
            return -2;  /* Duplicate plate — vehicle already parked */
        }
        current = current->next;
    }

    /* Allocate new entry */
    entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (!entry) return -1;

    strncpy(entry->plate, plate, 15);
    entry->plate[15] = '\0';
    entry->floor = floor;
    entry->slot = slot;
    entry->entry_time = entry_time;
    entry->is_vip = is_vip;

    /* Prepend to the chain (O(1) — new entry becomes the chain head) */
    entry->next = ht->buckets[bucket];
    ht->buckets[bucket] = entry;
    ht->count++;

    return 0;
}

/*
 * ht_lookup — Hash the plate, walk the chain, compare plates.
 *
 * Average case O(1) when load factor is low (count / capacity < 0.75).
 * Worst case O(n) if all entries hash to the same bucket — extremely
 * unlikely with DJB2 on license plate strings.
 */
HashEntry *ht_lookup(const HashTable *ht, const char *plate)
{
    unsigned long hash;
    int bucket;
    HashEntry *current;

    if (!ht || !plate) return NULL;

    hash = djb2_hash(plate);
    bucket = (int)(hash % (unsigned long)ht->capacity);

    current = ht->buckets[bucket];
    while (current) {
        if (strcmp(current->plate, plate) == 0) {
            return current;  /* Found */
        }
        current = current->next;
    }

    return NULL;  /* Not found */
}

/*
 * ht_delete — Find and unlink the entry from its chain, then free it.
 *
 * We maintain a 'prev' pointer to relink the chain after removing
 * the target node. Special case: if the target is the chain head,
 * we update the bucket pointer directly.
 */
int ht_delete(HashTable *ht, const char *plate)
{
    unsigned long hash;
    int bucket;
    HashEntry *current, *prev;

    if (!ht || !plate) return -1;

    hash = djb2_hash(plate);
    bucket = (int)(hash % (unsigned long)ht->capacity);

    current = ht->buckets[bucket];
    prev = NULL;

    while (current) {
        if (strcmp(current->plate, plate) == 0) {
            /* Found — unlink from chain */
            if (prev) {
                prev->next = current->next;
            } else {
                /* Target is the chain head — update bucket pointer */
                ht->buckets[bucket] = current->next;
            }
            free(current);
            ht->count--;
            return 0;
        }
        prev = current;
        current = current->next;
    }

    return -1;  /* Plate not found */
}

int ht_count(const HashTable *ht)
{
    return ht ? ht->count : 0;
}

/*
 * ht_destroy — Walk every bucket, free every chain node, then free buckets.
 */
void ht_destroy(HashTable *ht)
{
    int i;
    HashEntry *current, *next_entry;

    if (!ht || !ht->buckets) return;

    for (i = 0; i < ht->capacity; i++) {
        current = ht->buckets[i];
        while (current) {
            next_entry = current->next;
            free(current);
            current = next_entry;
        }
    }

    free(ht->buckets);
    ht->buckets = NULL;
    ht->count = 0;
}
