/*
 * linked_list.c — Singly Linked List for Vehicle Activity Log
 *
 * See linked_list.h for design rationale and API documentation.
 */

#include "../include/linked_list.h"
#include <stdlib.h>   /* malloc, free */
#include <string.h>   /* strncpy */

void ll_init(LinkedList *ll)
{
    if (!ll) return;
    ll->head = NULL;
    ll->tail = NULL;
    ll->count = 0;
}

/*
 * ll_append — Create a new LogEntry node and append it at the tail.
 *
 * Tail insertion keeps the list in chronological order (oldest at head,
 * newest at tail). By maintaining a tail pointer, append is O(1) —
 * we don't need to traverse the entire list.
 */
int ll_append(LinkedList *ll, const char *plate, const char *action,
              int floor, int slot, long timestamp, double amount)
{
    LogEntry *entry;

    if (!ll || !plate || !action) return -1;

    entry = (LogEntry *)malloc(sizeof(LogEntry));
    if (!entry) return -1;

    /* Copy fields into the new node */
    strncpy(entry->plate, plate, 15);
    entry->plate[15] = '\0';
    strncpy(entry->action, action, 7);
    entry->action[7] = '\0';
    entry->floor = floor;
    entry->slot = slot;
    entry->timestamp = timestamp;
    entry->amount = amount;
    entry->next = NULL;

    /* Append at the tail */
    if (ll->tail) {
        /* List is not empty — link after current tail */
        ll->tail->next = entry;
        ll->tail = entry;
    } else {
        /* List is empty — this node is both head and tail */
        ll->head = entry;
        ll->tail = entry;
    }

    ll->count++;
    return 0;
}

/*
 * ll_get_recent — Collect the last N entries from the list.
 *
 * Since this is a singly linked list (no backward pointers), we use a
 * two-pass approach:
 *   Pass 1: Count total entries and determine the start position.
 *   Pass 2: Walk to the start position, then collect entries.
 *
 * For a small log (< 1000 entries), this is perfectly acceptable.
 * A production system would use a circular buffer or doubly-linked list.
 */
int ll_get_recent(const LinkedList *ll, LogEntry **entries, int max_count)
{
    LogEntry *current;
    int skip, collected, i;

    if (!ll || !entries || max_count <= 0) return 0;

    /* How many entries to skip to get the last max_count */
    skip = (ll->count > max_count) ? (ll->count - max_count) : 0;

    /* Walk past the entries we're skipping */
    current = ll->head;
    for (i = 0; i < skip && current; i++) {
        current = current->next;
    }

    /* Collect pointers to the remaining entries */
    collected = 0;
    while (current && collected < max_count) {
        entries[collected] = current;
        collected++;
        current = current->next;
    }

    return collected;
}

int ll_count(const LinkedList *ll)
{
    return ll ? ll->count : 0;
}

/*
 * ll_destroy — Walk the list from head, freeing every node.
 */
void ll_destroy(LinkedList *ll)
{
    LogEntry *current, *next_entry;

    if (!ll) return;

    current = ll->head;
    while (current) {
        next_entry = current->next;
        free(current);
        current = next_entry;
    }

    ll->head = NULL;
    ll->tail = NULL;
    ll->count = 0;
}
