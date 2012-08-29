#include <stdio.h>
#include <string.h>

#include "array.h"

arr_t *
arr_new(int dims, int *len, int item_sz)
{
	arr_t *arr;
	int i, n;

	arr = malloc_or_die(sizeof(*arr));

	arr->dims = dims;

	arr->len = malloc_or_die(sizeof(*len) * (dims + 1));
	memcpy(arr->len, len, sizeof(*len) * (dims + 1));
	
	arr->item_sz = item_sz;
	
	n = 1;
	for (i = 0; i < dims; i++)
		n *= len[i];

	arr->ptr = malloc_or_die(item_sz * n);
	
	memset(arr->ptr, 0, item_sz * n);

	return arr;
}

//FIXME: be care when will introduce large numbers
ret_t
arr_set_item(arr_t *arr, int *ind, int value)
{
	int *p;
	int i, n;

	n = 1;

	for (i = 0; i < arr->dims; i++) {
		if (ind[i] >= arr->len[i] || ind[i] < 0)
			return ret_invalid;
		
		n *= ind[i];
	}
	
	p = arr->ptr + arr->item_sz * n;

	*p = value;

	return ret_ok;
}

ret_t
arr_get_item(arr_t *arr, int *ind, int *value)
{
	int *p;
	int i, n;

	n = 1;

	for (i = 0; i < arr->dims; i++) {
		if (ind[i] >= arr->len[i] || ind[i] < 0)
			return ret_invalid;
		
		n *= ind[i];
	}
	
	p = (int *)(arr->ptr + arr->item_sz * n);
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
	ufree(arr->len);
	ufree(arr->ptr);
	ufree(arr);
}


