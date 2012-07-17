#ifndef __STACK_H__
#define __STACK_H__

typedef void (*stack_item_free_t)(void *item);

void stack_push(void *data);

void *stack_pop();

void stack_flush(stack_item_free_t ifree);

#endif

