/*
 * hash_table.h — Hash Table for License Plate → Slot Lookup
 *
 * PURPOSE:
 *   Provides O(1) average-case lookup from a license plate string to
 *   the vehicle's parking location (floor + slot). This enables the
 *   "Where is my car?" feature and fast exit processing.
 *
 * IMPLEMENTATION:
 *   - Hash function: DJB2 (Dan Bernstein's hash) — simple, effective
 *     for short strings like license plates.
 *   - Collision resolution: Separate chaining — each bucket holds a
 *     linked list of entries that hash to the same index.
 *   - Load factor: If count/capacity > 0.75, performance degrades but
 *     remains correct. For a 60-slot garage, this is never an issue.
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef struct HashEntry {
    char plate[16];           /* License plate (key)                        */
    int  floor;               /* Floor where the vehicle is parked          */
    int  slot;                /* Slot number on that floor                  */
    long entry_time;          /* Unix timestamp of when the vehicle parked  */
    int  is_vip;              /* 1 if VIP, 0 otherwise                     */
    struct HashEntry *next;   /* Next entry in the chain (collision list)   */
} HashEntry;

typedef struct {
    HashEntry **buckets;      /* Array of pointers to chain heads           */
    int capacity;             /* Number of buckets                          */
    int count;                /* Number of entries currently stored         */
} HashTable;

/*
 * ht_init — Allocate and initialize the hash table.
 * @ht:       Pointer to HashTable struct.
 * @capacity: Number of buckets. A prime number is ideal (e.g., 101).
 *
 * Returns 0 on success, -1 on allocation failure.
 */
int ht_init(HashTable *ht, int capacity);

/*
 * ht_insert — Insert a vehicle record into the hash table.
 * @ht:         Pointer to the HashTable.
 * @plate:      License plate string (key).
 * @floor:      Floor number.
 * @slot:       Slot number.
 * @entry_time: Unix timestamp.
 * @is_vip:     1 for VIP, 0 for regular.
 *
 * If the plate already exists, returns -2 (duplicate).
 * Returns 0 on success, -1 on allocation failure.
 */
int ht_insert(HashTable *ht, const char *plate, int floor, int slot,
              long entry_time, int is_vip);

/*
 * ht_lookup — Find a vehicle by license plate.
 * @ht:    Pointer to the HashTable.
 * @plate: License plate string to search for.
 *
 * Returns pointer to the HashEntry if found, NULL otherwise.
 */
HashEntry *ht_lookup(const HashTable *ht, const char *plate);

/*
 * ht_delete — Remove a vehicle record from the hash table.
 * @ht:    Pointer to the HashTable.
 * @plate: License plate string to remove.
 *
 * Returns 0 on success, -1 if the plate was not found.
 */
int ht_delete(HashTable *ht, const char *plate);

/*
 * ht_count — Return the number of vehicles currently in the table.
 */
int ht_count(const HashTable *ht);

/*
 * ht_destroy — Free all entries and the bucket array.
 */
void ht_destroy(HashTable *ht);

#endif /* HASH_TABLE_H */
