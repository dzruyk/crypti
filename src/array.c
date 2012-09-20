#include <stdio.h>
#include <string.h>

#include "array.h"
#include "crypti.h"

arr_t *
arr_new(int dims, int *len, int sz, int item_sz)
{
	arr_t *arr;

	arr = xmalloc(sizeof(*arr));

	arr->dims = dims;

	arr->len = xmalloc(sizeof(*len) * dims);
	memcpy(arr->len, len, sizeof(*len) * dims);

	arr->sz = sz;
	
	arr->item_sz = item_sz;
	
	arr->ptr = xmalloc(item_sz * sz);
	
	memset(arr->ptr, 0, item_sz * sz);

	return arr;
}

arr_t *
arr_copy(arr_t *arr)
{
	arr_t *copy;
	int *parr, *pcopy;
	int i;

	copy = arr_new(arr->dims, arr->len, arr->sz, arr->item_sz);

	for (i = 0; i < copy->sz; i++) {
		//FIXME: Be care when you will work with
		//large numbers
		parr = (int *)(arr->ptr + arr->item_sz * i);
		pcopy = (int *)(copy->ptr + copy->item_sz * i);
		*pcopy = *parr;

	}

	return copy;
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
arr_print(arr_t *arr)
{
	int i, n, val;
	int *index;

#if IS_DEBUG == 1
	
	DEBUG(LOG_DEFAULT, "%d dims\n", arr->dims);
 	for (i = 0; i < arr->dims; i++)
		DEBUG(LOG_DEFAULT, "%d) len = %d\n", i, arr->len[i]);
#endif
		
	
#if IS_DEBUG == 1
	n = 1;
	for (i = 0; i < arr->dims; i++)
		n *= arr->len[i];
	
	DEBUG(LOG_DEFAULT, "n = %d\n", n);
#endif
	
	index = xmalloc(sizeof(*index) * arr->dims);
	memset(index, 0, sizeof(*index) * arr->dims);
	
	
	for (i = 0; i < arr->dims; i++)
		printf("{");

	while (1) {
		int is_end = 0;
		//print values at most depth
		while (index[arr->dims - 1] < arr->len[arr->dims - 1]) {
			arr_get_item(arr, index, &val);
			printf("%d", val);
			index[arr->dims - 1]++;
			if (index[arr->dims - 1] != arr->len[arr->dims - 1])
				printf(",");

		}

		index[arr->dims - 1] = 0;
	
		i = arr->dims - 2;
		n = 0;
		while (i >= 0) {
			n++;
			index[i]++;
			if (index[i] >= arr->len[i]) {
				index[i] = index[i] % arr->len[i];
				i--;
			} else {
				break;
			}
		}
		if (i < 0)
			is_end = 1;
		for (i = 0; i < n; i++)
			printf("}");
		if (is_end)
			break;
		printf(",");
		for (i = 0; i < n; i++)
			printf("{");
	}
	printf("}\n");

	ufree(index);
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

