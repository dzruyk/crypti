#include <assert.h>

#include <stdlib.h>

#include "stack.h"
#include "common.h"
#include "macros.h"


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
		error(1, "INTERNAL_ERROR: try to push NULL in stack\n");

	if (stack->cur + 1 == stack->sz)
		error(1, "INTERNAL_ERR: stack overflow\n");

	stack->arr[stack->cur++] = data;
}


void
stack_push_n(void **data, int n)
{
	int i;

	assert (stack != NULL &&
	    stack->sz > stack->cur + 1);

	for (i = 0; i < n; i++)
		stack_push(*data++);
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
stack_pop_n(void **ptr, int n)
{
	int i;

	for (i = 0; i < n; i++)
		*ptr++ = stack_pop();
}

/*
void *
stack_ptr()
{
	return stack->arr + stack->cur;
}
*/

void
stack_remove()
{

	assert (stack != NULL &&
	    stack->cur > 0);

	stack->cur --;
	stack->destructor(stack->arr[stack->cur]);
}

void
stack_remove_n(int n)
{
	while (n-- > 0)
		stack_remove();
}

void
stack_flush()
{
	int i;

	assert (stack != NULL &&
	    stack->sz > stack->cur + 1);

	for (i = 0; i < stack->cur; i++)
		stack->destructor(stack->arr[i]);

	stack->cur = 0;
}

