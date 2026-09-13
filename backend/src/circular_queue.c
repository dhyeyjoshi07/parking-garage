/*
 * circular_queue.c — Implementation of Circular Queue for Slot Allocation
 *
 * See circular_queue.h for design rationale and API documentation.
 */

#include "../include/circular_queue.h"
#include <stdlib.h>  /* malloc, free */

/*
 * cq_init — Allocate the internal array and pre-fill with slot indices.
 *
 * After initialization, the queue contains [0, 1, 2, ..., capacity-1],
 * meaning all slots on the floor are available. The front pointer starts
 * at index 0, and the rear pointer is at (capacity - 1).
 */
int cq_init(CircularQueue *cq, int capacity)
{
    int i;

    if (!cq || capacity <= 0) return -1;

    cq->data = (int *)malloc(sizeof(int) * capacity);
    if (!cq->data) return -1;

    cq->capacity = capacity;
    cq->front = 0;
    cq->rear = capacity - 1;
    cq->count = capacity;

    /*
     * Pre-fill the queue with slot indices so that all slots start as
     * "available." The first dequeue will return slot 0, then 1, etc.
     */
    for (i = 0; i < capacity; i++) {
        cq->data[i] = i;
    }

    return 0;
}

/*
 * cq_enqueue — Return a slot index to the available pool.
 *
 * The rear pointer advances by one, wrapping around using modulo.
 * This is the key trick of circular queues: (rear + 1) % capacity
 * wraps from the end of the array back to index 0.
 */
int cq_enqueue(CircularQueue *cq, int slot)
{
    if (!cq || cq->count == cq->capacity) return -1;

    /* Advance rear with wraparound */
    cq->rear = (cq->rear + 1) % cq->capacity;
    cq->data[cq->rear] = slot;
    cq->count++;

    return 0;
}

/*
 * cq_dequeue — Remove and return the next available slot index.
 *
 * The front pointer advances by one, also wrapping around.
 * The slot at the old front position is the one being allocated.
 */
int cq_dequeue(CircularQueue *cq, int *out_slot)
{
    if (!cq || !out_slot || cq->count == 0) return -1;

    *out_slot = cq->data[cq->front];

    /* Advance front with wraparound */
    cq->front = (cq->front + 1) % cq->capacity;
    cq->count--;

    return 0;
}

int cq_is_empty(const CircularQueue *cq)
{
    return (cq && cq->count == 0) ? 1 : 0;
}

int cq_is_full(const CircularQueue *cq)
{
    return (cq && cq->count == cq->capacity) ? 1 : 0;
}

int cq_available_count(const CircularQueue *cq)
{
    return cq ? cq->count : 0;
}

void cq_destroy(CircularQueue *cq)
{
    if (cq && cq->data) {
        free(cq->data);
        cq->data = NULL;
        cq->count = 0;
    }
}
