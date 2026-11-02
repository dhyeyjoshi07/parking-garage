/*
 * priority_queue.c — Min-Heap Priority Queue for VIP/Reserved Waitlist
 *
 * See priority_queue.h for design rationale and API documentation.
 *
 * HEAP INTERNALS:
 *   The heap is stored as a flat array where for any element at index i:
 *     - Parent:      (i - 1) / 2
 *     - Left child:  2*i + 1
 *     - Right child: 2*i + 2
 *
 *   The "smallest" element (highest real-world priority) is always at
 *   index 0. We compare by (priority, request_time) — lower priority
 *   number wins; ties broken by earlier timestamp.
 */

#include "../include/priority_queue.h"
#include <stdlib.h>   /* malloc, free */
#include <string.h>   /* memcpy, strncpy */

/* ---------- Internal helper: compare two waitlist entries ----------
 *
 * Returns 1 if 'a' should come BEFORE 'b' in the min-heap.
 * First compares priority (lower = higher real priority),
 * then breaks ties with request_time (earlier = higher priority).
 */
static int entry_less_than(const WaitlistEntry *a, const WaitlistEntry *b)
{
    if (a->priority != b->priority)
        return a->priority < b->priority;
    return a->request_time < b->request_time;
}

/* ---------- Internal helper: swap two entries in the heap ---------- */
static void swap_entries(WaitlistEntry *a, WaitlistEntry *b)
{
    WaitlistEntry tmp;
    memcpy(&tmp, a, sizeof(WaitlistEntry));
    memcpy(a, b, sizeof(WaitlistEntry));
    memcpy(b, &tmp, sizeof(WaitlistEntry));
}

/*
 * sift_up — Restore heap property after insertion.
 *
 * After inserting at the end of the array, the new element may be
 * "smaller" than its parent. We repeatedly swap it upward until
 * the parent is smaller or we reach the root.
 */
static void sift_up(PriorityQueue *pq, int idx)
{
    int parent;
    while (idx > 0) {
        parent = (idx - 1) / 2;
        if (entry_less_than(&pq->heap[idx], &pq->heap[parent])) {
            swap_entries(&pq->heap[idx], &pq->heap[parent]);
            idx = parent;
        } else {
            break;  /* Heap property satisfied */
        }
    }
}

/*
 * sift_down — Restore heap property after extraction.
 *
 * After moving the last element to the root, it may be "larger" than
 * its children. We repeatedly swap it with the smaller child until
 * both children are larger or we reach a leaf.
 */
static void sift_down(PriorityQueue *pq, int idx)
{
    int left, right, smallest;

    while (1) {
        left = 2 * idx + 1;
        right = 2 * idx + 2;
        smallest = idx;

        if (left < pq->size &&
            entry_less_than(&pq->heap[left], &pq->heap[smallest])) {
            smallest = left;
        }
        if (right < pq->size &&
            entry_less_than(&pq->heap[right], &pq->heap[smallest])) {
            smallest = right;
        }

        if (smallest != idx) {
            swap_entries(&pq->heap[idx], &pq->heap[smallest]);
            idx = smallest;
        } else {
            break;  /* Both children are larger, heap restored */
        }
    }
}

/* ==================== Public API ==================== */

int pq_init(PriorityQueue *pq, int capacity)
{
    if (!pq || capacity <= 0) return -1;

    pq->heap = (WaitlistEntry *)malloc(sizeof(WaitlistEntry) * capacity);
    if (!pq->heap) return -1;

    pq->size = 0;
    pq->capacity = capacity;
    return 0;
}

/*
 * pq_insert — Add an entry at the end of the array, then sift up.
 *
 * Time complexity: O(log n) due to sift_up.
 */
int pq_insert(PriorityQueue *pq, WaitlistEntry entry)
{
    if (!pq || pq->size >= pq->capacity) return -1;

    /* Place at the end of the heap array */
    memcpy(&pq->heap[pq->size], &entry, sizeof(WaitlistEntry));
    pq->size++;

    /* Restore heap property by sifting the new element up */
    sift_up(pq, pq->size - 1);

    return 0;
}

/*
 * pq_extract_min — Copy the root (min), move last to root, sift down.
 *
 * Time complexity: O(log n) due to sift_down.
 */
int pq_extract_min(PriorityQueue *pq, WaitlistEntry *out_entry)
{
    if (!pq || !out_entry || pq->size == 0) return -1;

    /* Copy the minimum element (root) to output */
    memcpy(out_entry, &pq->heap[0], sizeof(WaitlistEntry));

    /* Move the last element to the root position */
    pq->size--;
    if (pq->size > 0) {
        memcpy(&pq->heap[0], &pq->heap[pq->size], sizeof(WaitlistEntry));
        /* Restore heap property by sifting the new root down */
        sift_down(pq, 0);
    }

    return 0;
}

const WaitlistEntry *pq_peek(const PriorityQueue *pq)
{
    if (!pq || pq->size == 0) return NULL;
    return &pq->heap[0];
}

int pq_is_empty(const PriorityQueue *pq)
{
    return (pq && pq->size == 0) ? 1 : 0;
}

int pq_size(const PriorityQueue *pq)
{
    return pq ? pq->size : 0;
}

void pq_destroy(PriorityQueue *pq)
{
    if (pq && pq->heap) {
        free(pq->heap);
        pq->heap = NULL;
        pq->size = 0;
    }
}
