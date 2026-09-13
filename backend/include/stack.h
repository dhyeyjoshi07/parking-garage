/*
 * stack.h — Stack for Single-Lane Exit Simulation
 *
 * PURPOSE:
 *   Models a single-lane garage exit where cars line up to leave.
 *   Since only the car at the front of the lane (top of stack) can
 *   physically drive out, this is a natural LIFO (Last-In, First-Out)
 *   structure.
 *
 * IMPLEMENTATION:
 *   Singly linked list with push/pop at the head (O(1) operations).
 */

#ifndef STACK_H
#define STACK_H

typedef struct StackNode {
    char plate[16];           /* License plate of the vehicle in the lane  */
    struct StackNode *next;   /* Pointer to the car behind this one        */
} StackNode;

typedef struct {
    StackNode *top;           /* Top of the stack (car nearest the exit)   */
    int size;                 /* Number of cars currently in the exit lane */
} Stack;

/*
 * stack_init — Initialize an empty stack.
 */
void stack_init(Stack *s);

/*
 * stack_push — A vehicle enters the exit lane.
 * @s:     Pointer to the Stack.
 * @plate: License plate string of the vehicle.
 *
 * Allocates a new node, links it at the top.
 * Returns 0 on success, -1 on allocation failure.
 */
int stack_push(Stack *s, const char *plate);

/*
 * stack_pop — The top vehicle exits the lane.
 * @s:         Pointer to the Stack.
 * @out_plate: Buffer (at least 16 bytes) to receive the plate string.
 *
 * Removes the top node and copies its plate to out_plate.
 * Returns 0 on success, -1 if the stack is empty.
 */
int stack_pop(Stack *s, char *out_plate);

/*
 * stack_peek — View the top vehicle without removing it.
 * Returns pointer to the plate string, or NULL if empty.
 */
const char *stack_peek(const Stack *s);

/*
 * stack_is_empty — Check if the exit lane is empty.
 */
int stack_is_empty(const Stack *s);

/*
 * stack_size — Return the number of vehicles in the exit lane.
 */
int stack_size(const Stack *s);

/*
 * stack_destroy — Free all nodes in the stack.
 */
void stack_destroy(Stack *s);

#endif /* STACK_H */
