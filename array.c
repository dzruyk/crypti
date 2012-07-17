#include <stdio.h>

#include "array.h"

arr_t *
arr_new(int n, int item_sz)
{
	arr_t *arr;

	arr = malloc_or_die(sizeof(*arr));

	arr->n = n;
	arr->item_sz = item_sz;
	arr->ptr = malloc_or_die(item_sz * n);

	return arr;
}

//WARNING: be care when will introduce large numbers
ret_t
arr_set_item(arr_t *arr, int ind, int value)
{
	int *p;

	if (ind >= arr->n || ind < 0)
		return ret_invalid;
	
	p = arr->ptr + arr->item_sz * ind;

	*p = value;

	return ret_ok;
}

ret_t
arr_get_item(arr_t *arr, int ind, int *value)
{
	int *p;

	if (ind >= arr->n || ind < 0)
		return ret_invalid;
		
	p = (int *)(arr->ptr + arr->item_sz * ind);
	*value = *p;

	return ret_ok;
}

void
arr_free(arr_t *arr)
{
	free(arr->ptr);
	free(arr);
}

void
arr_print(arr_t *arr)
{
	int i;
	int *parr;

	parr = arr->ptr;

	for (i = 0; i < arr->n; i++)
		printf("%d ", parr[i]);
	printf("\n");
}

