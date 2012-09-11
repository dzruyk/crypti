#ifndef __STACK_H__
#define __STACK_H__

enum {
	stack_sz = 128,
};

typedef void (*stack_destructor)(void *data);

void stack_init(stack_destructor destructor);

void stack_destroy();

void stack_push(void *data);

void *stack_pop();

void stack_flush();

#endif

