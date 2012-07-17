#include <stdlib.h>

#include "stack.h"
#include "common.h"

//FIXME: may be need to implement stack with dynamic array
//	 but I'm so tired...)

struct stack {
	struct stack *next;
	void *data;
};

struct stack *stack = NULL;

void 
stack_push(void *data)
{
	struct stack *nitem;

	if (data == NULL)
		print_warn_and_die("INTERNAL_ERROR: try to push NULL in stack\n");
	
	nitem = malloc_or_die(sizeof(*nitem));
	
	nitem->data = data;
	nitem->next = stack;
	stack = nitem;
}

void *
stack_pop()
{
	struct stack *nitem;
	void *data;

	if (stack == NULL)
		return NULL;
	
	nitem = stack;
	stack = stack->next;

	data = nitem->data;
	
	free(nitem);
	return data;
}

void 
stack_flush(stack_item_free_t ifree)
{
	struct stack *next;

	for (; stack != NULL; stack = next) {
		next = stack->next;
		ifree(stack->data);
		free(stack);
	}
}

