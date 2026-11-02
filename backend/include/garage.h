/*
 * garage.h — Core Parking Garage Manager
 *
 * PURPOSE:
 *   This is the orchestration layer that ties together all five data
 *   structures to implement the parking garage business logic:
 *
 *   - CircularQueue (per floor) → track available slots
 *   - PriorityQueue            → waitlist with VIP priority
 *   - HashTable                → plate-to-location O(1) lookup
 *   - LinkedList               → chronological activity log
 *   - Stack                    → exit lane simulation
 *
 *   The Garage struct holds the entire state and can be saved/loaded
 *   to/from a binary file for persistence across CLI invocations.
 */

#ifndef GARAGE_H
#define GARAGE_H

#include "circular_queue.h"
#include "priority_queue.h"
#include "stack.h"
#include "linked_list.h"
#include "hash_table.h"

/* Configuration constants */
#define NUM_FLOORS      3
#define SLOTS_PER_FLOOR 20
#define TOTAL_SLOTS     (NUM_FLOORS * SLOTS_PER_FLOOR)
#define MAX_WAITLIST    50
#define HASH_BUCKETS    101   /* Prime number for better distribution */

/* Result codes returned by garage operations */
#define GARAGE_OK              0
#define GARAGE_ERR_FULL       -1   /* All floors full, added to waitlist   */
#define GARAGE_ERR_NOT_FOUND  -2   /* Vehicle not found in the garage     */
#define GARAGE_ERR_DUPLICATE  -3   /* Vehicle already parked              */
#define GARAGE_ERR_WAITLIST   -4   /* Added to waitlist (not an error)    */
#define GARAGE_ERR_ALLOC      -5   /* Memory allocation failure           */
#define GARAGE_ERR_EMPTY_LANE -6   /* Exit lane is empty                  */

/* Holds the result of a park operation */
typedef struct {
    int result_code;       /* GARAGE_OK or GARAGE_ERR_*                    */
    int floor;             /* Floor assigned (-1 if waitlisted)            */
    int slot;              /* Slot assigned (-1 if waitlisted)             */
    int waitlist_pos;      /* Position in waitlist (-1 if parked)          */
    char message[128];     /* Human-readable result message                */
} ParkResult;

/* Holds the result of an exit operation */
typedef struct {
    int result_code;
    char plate[16];
    int floor;
    int slot;
    long entry_time;
    long exit_time;
    long duration_seconds;
    double amount;
    int is_vip;
    char duration_str[32]; /* e.g., "2h 15m" */
    char message[128];
} ExitResult;

/* Holds the result of a lookup operation */
typedef struct {
    int result_code;
    char plate[16];
    int floor;
    int slot;
    long entry_time;
    int is_vip;
    char message[128];
} LookupResult;

/*
 * The main Garage structure — holds all state.
 */
typedef struct {
    CircularQueue floors[NUM_FLOORS]; /* One slot queue per floor          */
    PriorityQueue waitlist;           /* VIP-prioritized waitlist          */
    HashTable     vehicles;           /* Plate → location lookup           */
    LinkedList    log;                /* Chronological event log           */
    Stack         exit_lane;          /* Single-lane exit stack            */
    int           total_revenue_cents;/* Running total (in cents)          */
    int           initialized;       /* Magic number to verify state file */
} Garage;

/*
 * garage_init — Initialize a fresh garage with all floors empty.
 * Returns 0 on success, -1 on failure.
 */
int garage_init(Garage *g);

/*
 * garage_park — Park a vehicle in the nearest available slot.
 * @g:      Pointer to Garage.
 * @plate:  License plate string.
 * @is_vip: 1 for VIP, 0 for regular.
 * @result: Output — filled with parking details.
 *
 * Logic:
 *   1. Check if vehicle is already parked (hash lookup).
 *   2. Try each floor's circular queue for an available slot.
 *   3. If all floors full, add to priority waitlist.
 */
void garage_park(Garage *g, const char *plate, int is_vip, ParkResult *result);

/*
 * garage_exit — Remove a vehicle from the garage and compute billing.
 * @g:      Pointer to Garage.
 * @plate:  License plate of the vehicle exiting.
 * @result: Output — filled with exit/billing details.
 *
 * Logic:
 *   1. Look up the vehicle in the hash table.
 *   2. Free the slot (enqueue back to the floor's circular queue).
 *   3. Calculate the charge based on duration.
 *   4. Log the exit event.
 *   5. If vehicles are on the waitlist, admit the highest-priority one.
 */
void garage_exit(Garage *g, const char *plate, ExitResult *result);

/*
 * garage_lookup — Find where a vehicle is parked.
 */
void garage_lookup(Garage *g, const char *plate, LookupResult *result);

/*
 * garage_destroy — Free all dynamically allocated memory.
 */
void garage_destroy(Garage *g);

/*
 * State persistence — save/load the entire garage state to/from a file.
 * The state file path defaults to "garage.dat" in the working directory.
 */
#define STATE_FILE "garage.dat"
#define GARAGE_MAGIC 0x50415243  /* "PARC" in hex — validates state files */

int garage_save(const Garage *g, const char *filepath);
int garage_load(Garage *g, const char *filepath);

#endif /* GARAGE_H */
