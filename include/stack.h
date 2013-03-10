#ifndef __STACK_H__
#define __STACK_H__

enum {
	stack_sz = 128,
};

typedef void (*stack_destructor)(void *data);

void stack_init(stack_destructor destructor);

void stack_destroy();

void stack_push(void *data);
void stack_push_n(void **data, int n);

void *stack_pop();
void stack_pop_n(void **ptr, int n);

void stack_remove();
void stack_remove_n(int n);
void stack_flush();

#endif

