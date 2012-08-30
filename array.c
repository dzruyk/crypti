#include <stdio.h>
#include <string.h>

#include "array.h"

arr_t *
arr_new(int dims, int *len, int sz, int item_sz)
{
	arr_t *arr;

	arr = malloc_or_die(sizeof(*arr));

	arr->dims = dims;

	arr->len = malloc_or_die(sizeof(*len) * dims);
	memcpy(arr->len, len, sizeof(*len) * dims);

	arr->sz = sz;
	
	arr->item_sz = item_sz;
	
	arr->ptr = malloc_or_die(item_sz * sz);
	
	memset(arr->ptr, 0, item_sz * sz);

	return arr;
}

//FIXME: be care when will introduce large numbers
ret_t
arr_set_item(arr_t *arr, int *ind, int value)
{
	int *p;
	int i, mult, n;

	mult = 1;
	n = 0;

	//ERROR: INVALID
	for (i = arr->dims - 1; i >= 0; i--) {
		if (ind[i] >= arr->len[i] || ind[i] < 0)
			return ret_invalid;
		n += ind[i] * mult;
		mult *= arr->len[i];
	}
	
	p = arr->ptr + arr->item_sz * n;

	*p = value;

	return ret_ok;
}

ret_t
arr_get_item(arr_t *arr, int *ind, int *value)
{
	int *p;
	int i, mult, n;

	mult = 1;
	n = 0;

	for (i = arr->dims - 1; i >= 0; i--) {
		if (ind[i] >= arr->len[i] || ind[i] < 0)
			return ret_invalid;
		
		n += ind[i] * mult;
		mult *= arr->len[i];
	}
	
	p = (int *)(arr->ptr + arr->item_sz * n);
	*value = *p;

	return ret_ok;
}

void
arr_free(arr_t *arr)
{
	/*
	for (i = 0; i < arr->n; i++)
		ufree(arr->ptr[i]);
	*/
	ufree(arr->len);
	ufree(arr->ptr);
	ufree(arr);
}


