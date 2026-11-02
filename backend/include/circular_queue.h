/*
 * circular_queue.h — Circular Queue for Parking Slot Allocation
 *
 * PURPOSE:
 *   Each floor of the parking garage has a fixed number of slots.
 *   This circular queue tracks AVAILABLE slot indices for a floor.
 *   - When a car parks:  dequeue → get the next free slot index.
 *   - When a car exits:  enqueue → return the slot index to the pool.
 *   - When count == 0:   floor is full → vehicles go to the waitlist.
 *
 * WHY CIRCULAR:
 *   A linear queue wastes space as the front pointer advances. The circular
 *   design wraps around, reusing vacated positions without shifting elements.
 */

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

typedef struct {
    int *data;      /* Dynamically allocated array of slot indices            */
    int front;      /* Index of the next element to dequeue                   */
    int rear;       /* Index of the last enqueued element                     */
    int capacity;   /* Maximum number of elements (= total slots on a floor)  */
    int count;      /* Current number of available slots in the queue         */
} CircularQueue;

/*
 * cq_init — Allocate and initialize a circular queue.
 * @cq:       Pointer to the CircularQueue struct to initialize.
 * @capacity: Maximum number of slots (elements) the queue can hold.
 *
 * After init, the queue is pre-filled with slot indices [0, 1, ..., capacity-1]
 * so that all slots start as "available."
 *
 * Returns 0 on success, -1 on allocation failure.
 */
int cq_init(CircularQueue *cq, int capacity);

/*
 * cq_enqueue — Add a slot index back to the available pool.
 * @cq:   Pointer to the CircularQueue.
 * @slot: The slot index being returned (e.g., after a car exits).
 *
 * Returns 0 on success, -1 if the queue is already full.
 */
int cq_enqueue(CircularQueue *cq, int slot);

/*
 * cq_dequeue — Remove and return the next available slot index.
 * @cq:       Pointer to the CircularQueue.
 * @out_slot: Output pointer — receives the dequeued slot index.
 *
 * Returns 0 on success, -1 if the queue is empty (floor is full).
 */
int cq_dequeue(CircularQueue *cq, int *out_slot);

/*
 * cq_is_empty — Check if no slots are available.
 * Returns 1 if empty, 0 otherwise.
 */
int cq_is_empty(const CircularQueue *cq);

/*
 * cq_is_full — Check if all slots are available (no cars parked).
 * Returns 1 if full (all slots free), 0 otherwise.
 */
int cq_is_full(const CircularQueue *cq);

/*
 * cq_available_count — Return the number of currently available slots.
 */
int cq_available_count(const CircularQueue *cq);

/*
 * cq_destroy — Free the dynamically allocated internal array.
 */
void cq_destroy(CircularQueue *cq);

#endif /* CIRCULAR_QUEUE_H */
