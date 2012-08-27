#include <stdio.h>
#include <string.h>

#include "array.h"

arr_t *
arr_new(int n, int item_sz)
{
	arr_t *arr;

	arr = malloc_or_die(sizeof(*arr));

	arr->n = n;
	arr->item_sz = item_sz;
	arr->ptr = malloc_or_die(item_sz * n);
	
	memset(arr->ptr, 0, item_sz * n);

	return arr;
}

//FIXME: be care when will introduce large numbers
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
	int i;

	/*
	for (i = 0; i < arr->n; i++)
		ufree(arr->ptr[i]);
	*/
	ufree(arr->ptr);
	ufree(arr);
}


