#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "list.h"

struct list *
list_init()
{
	struct list *lst;
	
	lst = malloc(sizeof(*lst));
	if (lst == NULL)
		return NULL;
	memset(lst, 0, sizeof(*lst));
	return lst;
}

struct list_item *
list_item_new(const void *data)
{
	struct list_item *tmp;
	
	tmp = malloc(sizeof(*tmp));
	if (tmp == NULL)
		return NULL;
	
	tmp->next = NULL;
	tmp->data = (void*)data;
	
	return tmp;
}


void
list_item_add(struct list *root, struct list_item *item)
{
	assert(item != NULL && root != NULL);
	
	if (root->list == NULL) {
		root->list = item;
		item->next = NULL;
	}
	else {
		item->next = root->list;
		root->list = item;
	}
}


void
list_item_add_to_end(struct list *root, struct list_item *item)
{
	assert(item != NULL && root != NULL);
	
	struct list_item *tmp;
	while ((tmp = root->list) != NULL)
		tmp = tmp->next;

	tmp->next = item;
	item->next = NULL;
}


void
list_reverse(struct list *root)
{
	assert(root != NULL);
	
	struct list_item *root2, *tmp, *iter;
	
	iter = root2 = NULL;
	
	for (iter = root->list; iter != NULL; iter = tmp) {
		tmp = iter->next;
		iter->next = root2;
		root2 = iter;
	}
	root->list = root2;
}

void
list_destroy(struct list **root, data_destroy_func_t func)
{
	assert( root != NULL);
	
	if (*root == NULL || (*root)->list == NULL)
		return;
	
	struct list_item *tmp, *last;

	last = (*root)->list;
	
	for (tmp = last->next; tmp != NULL; tmp = tmp->next) {
		func(last->data);
		free(last);
		last = tmp;
	}
	
	func(last->data);
	free(last);

	free(*root);
	
	*root = NULL;
}


struct list_item 
*list_find(const struct list *root, const void *data, compare_func_t func)
{
	struct list_item *tmp;

	for (tmp = root->list; tmp != NULL; tmp = tmp->next) {
		if (func(data, tmp->data) == 0)
			return tmp;
	}

	return NULL;
}


int
list_pass(struct list *root, list_pass_cb func, void *data)
{
	struct list_item *tmp;
	int i = 0;

	for (tmp = root->list; tmp != NULL; tmp = tmp->next, i++)
		if (func != NULL)
			func(tmp, data);

	return i;
}


struct stack_item {
	int n;			//determines sizeof list
	struct list_item *root;
};

/*
 * Сортирует 2 уже отсортированных списка left и right длины n  и помещает
 * результат длины 2n в left root
 */
static void
stack_item_sort(struct stack_item *left, struct stack_item *right, compare_func_t func)
{
	assert(left != NULL && right != NULL);
	
	struct list_item *root, *tmp;
	struct list_item *ll, *rl;
	int res;
	
	ll = left->root;
	rl = right->root;

	res = func(ll->data, rl->data);
	if (res >= 0) {
		root = ll;
		ll = ll->next;
	} else {
		root = rl;
		rl = rl->next;
	}
	
	tmp = root;
	while (ll != NULL && rl != NULL) {
		res = func(ll->data, rl->data);
		if (res >= 0) {
			tmp->next = ll;
			ll = ll->next;
		} else {
			tmp->next = rl;
			rl = rl->next;
		}
		tmp = tmp->next;
	}
	
	if (ll != NULL) {
		tmp->next = ll;
		tmp = tmp->next;
	}
	else if (rl != NULL) {
		tmp->next = rl;
		tmp = tmp->next;
	}
	else
		tmp->next = NULL;
	
	left->n += right->n;
	left->root = root;
	right->n = 0;
	right->root = NULL;
}

static int
lb(int a)
{
	assert(a >= 0);

	int i;
	
	for (i = 0; a > 1; a >>= 1)
		++i;
	return i;
}

/*
 * sort alg is recursive
 * it needs array size = log2(list_len) + 1 
 *
 *
 */
int
list_sort(struct list *root, compare_func_t func)
{
	struct stack_item *stack;
	struct list_item *tmp;
	int spos, list_len, stack_len;

	list_len = list_pass(root, NULL, NULL);
	
	if (list_len < 2)		//no data to sort
		return 0;
	
	stack_len = lb(list_len) + 1;

	stack = malloc(stack_len * sizeof(*stack)); //Непровереный маллок
	if (stack == NULL)
		return -1;
	
	memset(stack, 0, sizeof(stack));
	
	spos = 0;
	tmp = root->list;
	while (tmp != NULL) {
		stack[spos].n = 1;
		stack[spos].root = tmp;

		tmp = tmp->next;
		stack[spos].root->next = NULL;
		spos++;
		
		while (spos > 1 && stack[spos - 1].n == stack[spos - 2].n) {
			stack_item_sort(&stack[spos - 2], &stack[spos - 1], func);
			spos--;
		}
	}
	
	while (spos > 1) {
		stack_item_sort(&stack[spos - 2], &stack[spos - 1], func);
		spos--;
	}

	root->list = stack[0].root;
	
	printf("list pass res = %d\n", list_pass(root, NULL, NULL));
	
	free(stack);
	return 0;
}

int
list_sort_simple(struct list *lst, compare_func_t func)
{
	struct list_item *root, *prev, *tmp;
	int end = 0, res;

	if (lst->list == NULL && lst->list->next == NULL)		//нечего сортировать
		return 0;

	root = lst->list;
	while (end != 1) {
		res = func(root->data, root->next->data);
		if (res < 0) {
			tmp = root->next;
			root->next = tmp->next;
			tmp->next = root;
			root = tmp;
		}
		tmp = root->next;
		prev = root;
		end = 1;
		
		while (tmp->next != NULL) {
			res = func(tmp->data, tmp->next->data);
			if (res < 0) {
				prev->next = tmp->next;
				tmp->next = tmp->next->next;
				prev->next->next = tmp;
				
				end = 0;
				prev = prev->next;
				
			} else {
				prev = prev->next;
				tmp = tmp->next;
			}
		}
	}
	
	lst->list = root;
	
	return 0;
}

