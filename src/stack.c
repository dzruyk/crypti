#include <assert.h>

#include <stdlib.h>

#include "stack.h"
#include "common.h"


struct stack {
	int cur;
	int sz;
	void **arr;
	stack_destructor destructor;
};

struct stack *stack = NULL;

void
stack_init(stack_destructor destructor)
{
	assert(stack == NULL);
	
	stack = xmalloc(sizeof(*stack));

	stack->arr = xmalloc(sizeof(*stack->arr) * stack_sz);
	stack->cur = 0;
	stack->sz = stack_sz;
	stack->destructor = destructor;
}

void
stack_destroy()
{
	assert (stack != NULL &&
	    stack->sz > stack->cur + 1);

	int i;

	for (i = 0; i < stack->cur; i++)
		stack->destructor(stack->arr[i]);

	ufree(stack->arr);
	ufree(stack);

	stack = NULL;
}

void 
stack_push(void *data)
{
	assert (stack != NULL &&
	    stack->sz > stack->cur + 1);

	if (data == NULL)
		print_warn_and_die("INTERNAL_ERROR: try to push NULL in stack\n");

	if (stack->cur + 1 == stack->sz)
		print_warn_and_die("INTERNAL_ERR: stack overflow\n");

	stack->arr[stack->cur++] = data;
}

void *
stack_pop()
{
	assert (stack != NULL &&
	    stack->sz > stack->cur + 1);

	if (stack->cur == 0)
		return NULL;

	stack->cur--;

	return stack->arr[stack->cur];
}

void 
stack_flush()
{
	assert (stack != NULL && 
	    stack->sz > stack->cur + 1);

	int i;

	for (i = 0; i < stack->cur; i++)
		stack->destructor(stack->arr[i]);

	stack->cur = 0;
}

