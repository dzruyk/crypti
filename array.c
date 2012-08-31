#include <stdio.h>
#include <string.h>

#include "array.h"
#include "crypti.h"

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
arr_print(arr_t *arr)
{
	int i, n, val;
	int *index;

	D(printf("%d dims\n", arr->dims);
 	  for (i = 0; i < arr->dims; i++)
		printf("%d) len = %d\n", i, arr->len[i]);
	)
		
	n = 1;

	for (i = 0; i < arr->dims; i++)
		n *= arr->len[i];
	
	//D(printf("n = %d\n", n));
	
	index = malloc_or_die(sizeof(*index) * arr->dims);
	memset(index, 0, sizeof(*index) * arr->dims);
	
	
	for (i = 0; i < arr->dims; i++)
		printf("{");

	while (1) {

		while (index[arr->dims - 1] < arr->len[arr->dims - 1]) {
			arr_get_item(arr, index, &val);
			printf(" %d,", val);
			index[arr->dims - 1]++;
		}

		index[arr->dims - 1] = 0;
	
		i = arr->dims - 2;
		while (i >= 0) {
			printf("}");
			index[i]++;
			if (index[i] >= arr->len[i]) {
				index[i] = index[i] % arr->len[i];
				i--;
			} else {
				break;
			}
		}
		if (i < 0)
			break;
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


