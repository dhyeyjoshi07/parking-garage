/*
 * stack.c — Linked-List Stack for Exit-Lane Simulation
 *
 * See stack.h for design rationale and API documentation.
 *
 * All operations (push, pop, peek) are O(1) since we only ever
 * manipulate the head of the linked list.
 */

#include "../include/stack.h"
#include <stdlib.h>   /* malloc, free */
#include <string.h>   /* strncpy */

void stack_init(Stack *s)
{
    if (!s) return;
    s->top = NULL;
    s->size = 0;
}

/*
 * stack_push — Allocate a new node, copy the plate, link at the top.
 *
 * The new node's 'next' points to the current top, then the new node
 * becomes the top. This is the standard linked-list push.
 */
int stack_push(Stack *s, const char *plate)
{
    StackNode *node;

    if (!s || !plate) return -1;

    node = (StackNode *)malloc(sizeof(StackNode));
    if (!node) return -1;

    strncpy(node->plate, plate, 15);
    node->plate[15] = '\0';  /* Ensure null termination */

    /* Link new node at the top */
    node->next = s->top;
    s->top = node;
    s->size++;

    return 0;
}

/*
 * stack_pop — Copy the top node's plate to output, unlink and free it.
 *
 * We save the top pointer, advance top to top->next, then free
 * the old top node. Classic linked-list pop.
 */
int stack_pop(Stack *s, char *out_plate)
{
    StackNode *old_top;

    if (!s || !out_plate || !s->top) return -1;

    old_top = s->top;
    strncpy(out_plate, old_top->plate, 15);
    out_plate[15] = '\0';

    /* Advance top and free the old node */
    s->top = old_top->next;
    free(old_top);
    s->size--;

    return 0;
}

const char *stack_peek(const Stack *s)
{
    if (!s || !s->top) return NULL;
    return s->top->plate;
}

int stack_is_empty(const Stack *s)
{
    return (s && s->top == NULL) ? 1 : 0;
}

int stack_size(const Stack *s)
{
    return s ? s->size : 0;
}

/*
 * stack_destroy — Walk the list, freeing every node.
 */
void stack_destroy(Stack *s)
{
    StackNode *current, *next_node;

    if (!s) return;

    current = s->top;
    while (current) {
        next_node = current->next;
        free(current);
        current = next_node;
    }

    s->top = NULL;
    s->size = 0;
}
