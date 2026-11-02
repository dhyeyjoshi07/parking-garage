/*
 * linked_list.h — Singly Linked List for Vehicle Activity Log
 *
 * PURPOSE:
 *   Maintains a chronological log of all parking events — every park,
 *   exit, and billing record. New entries are appended at the tail
 *   for chronological ordering. The frontend reads this log to display
 *   recent activity.
 *
 * IMPLEMENTATION:
 *   Singly linked list with both head and tail pointers for O(1) append.
 */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct LogEntry {
    char plate[16];           /* License plate                              */
    char action[8];           /* "park" or "exit"                           */
    int  floor;               /* Floor number (0-indexed)                   */
    int  slot;                /* Slot number on that floor                  */
    long timestamp;           /* Unix timestamp of the event                */
    double amount;            /* Amount charged (0.0 for park events)       */
    struct LogEntry *next;    /* Pointer to the next log entry              */
} LogEntry;

typedef struct {
    LogEntry *head;           /* Oldest log entry                           */
    LogEntry *tail;           /* Newest log entry (for O(1) append)         */
    int count;                /* Total number of log entries                */
} LinkedList;

/*
 * ll_init — Initialize an empty linked list.
 */
void ll_init(LinkedList *ll);

/*
 * ll_append — Add a new log entry at the tail.
 * @ll:        Pointer to the LinkedList.
 * @plate:     License plate string.
 * @action:    "park" or "exit".
 * @floor:     Floor number.
 * @slot:      Slot number.
 * @timestamp: Unix timestamp of the event.
 * @amount:    Billing amount (0.0 for park events).
 *
 * Returns 0 on success, -1 on allocation failure.
 */
int ll_append(LinkedList *ll, const char *plate, const char *action,
              int floor, int slot, long timestamp, double amount);

/*
 * ll_get_recent — Get the N most recent log entries.
 * @ll:        Pointer to the LinkedList.
 * @entries:   Output array of LogEntry pointers (caller provides array).
 * @max_count: Maximum number of entries to retrieve.
 *
 * Returns the actual number of entries written to the array.
 *
 * NOTE: Since this is a singly linked list, getting the "last N" requires
 * traversing from head. For a small log this is fine. For production,
 * you'd use a doubly-linked list or maintain a circular buffer.
 */
int ll_get_recent(const LinkedList *ll, LogEntry **entries, int max_count);

/*
 * ll_count — Return the total number of log entries.
 */
int ll_count(const LinkedList *ll);

/*
 * ll_destroy — Free all nodes in the linked list.
 */
void ll_destroy(LinkedList *ll);

#endif /* LINKED_LIST_H */
