/*
 * priority_queue.h — Min-Heap Priority Queue for VIP/Reserved Waitlist
 *
 * PURPOSE:
 *   When all parking floors are full, incoming vehicles join a waitlist.
 *   VIP vehicles (priority=0) should be admitted before reserved (priority=1)
 *   and regular vehicles (priority=2). Within the same priority level,
 *   earlier arrivals go first (FIFO tie-breaking via request_time).
 *
 * IMPLEMENTATION:
 *   A min-heap where the "smallest" element has the lowest (priority, time)
 *   tuple. Extract-min always gives us the next vehicle to admit.
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

/* Priority levels — lower number = higher priority */
#define PRIORITY_VIP      0
#define PRIORITY_RESERVED 1
#define PRIORITY_REGULAR  2

typedef struct {
    char plate[16];       /* License plate string                       */
    int  priority;        /* 0=VIP, 1=reserved, 2=regular               */
    long request_time;    /* Unix timestamp — FIFO within same priority  */
    int  is_vip;          /* 1 if VIP, 0 otherwise (for billing)        */
} WaitlistEntry;

typedef struct {
    WaitlistEntry *heap;  /* Dynamically allocated array (heap storage)  */
    int size;             /* Current number of entries in the heap       */
    int capacity;         /* Maximum heap capacity                       */
} PriorityQueue;

/*
 * pq_init — Allocate and initialize the priority queue.
 * @pq:       Pointer to PriorityQueue struct.
 * @capacity: Maximum number of waitlist entries.
 *
 * Returns 0 on success, -1 on allocation failure.
 */
int pq_init(PriorityQueue *pq, int capacity);

/*
 * pq_insert — Add a vehicle to the waitlist.
 * @pq:    Pointer to the PriorityQueue.
 * @entry: The waitlist entry to insert.
 *
 * Performs sift-up to maintain the min-heap invariant.
 * Returns 0 on success, -1 if the queue is full.
 */
int pq_insert(PriorityQueue *pq, WaitlistEntry entry);

/*
 * pq_extract_min — Remove and return the highest-priority vehicle.
 * @pq:        Pointer to the PriorityQueue.
 * @out_entry: Output pointer — receives the extracted entry.
 *
 * Swaps root with last element, then sift-down to restore heap.
 * Returns 0 on success, -1 if the queue is empty.
 */
int pq_extract_min(PriorityQueue *pq, WaitlistEntry *out_entry);

/*
 * pq_peek — View the highest-priority vehicle without removing it.
 * Returns pointer to the root entry, or NULL if empty.
 */
const WaitlistEntry *pq_peek(const PriorityQueue *pq);

/*
 * pq_is_empty — Check if the waitlist is empty.
 */
int pq_is_empty(const PriorityQueue *pq);

/*
 * pq_size — Return the current number of vehicles in the waitlist.
 */
int pq_size(const PriorityQueue *pq);

/*
 * pq_destroy — Free the dynamically allocated heap array.
 */
void pq_destroy(PriorityQueue *pq);

#endif /* PRIORITY_QUEUE_H */
