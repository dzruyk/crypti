#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "log.h"

arr_t *
arr_new(int dims, int *len, int sz, int item_sz)
{
	arr_t *arr;

	assert(dims > 0 && len != NULL && item_sz > 0);

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
arr_copy(arr_t *arr, arr_item_copy_t f)
{
	arr_t *copy;
	void **parr, **pcopy;
	int i;

	assert(arr != NULL && f != NULL);

	copy = arr_new(arr->dims, arr->len, arr->sz, arr->item_sz);

	for (i = 0; i < copy->sz; i++) {
		//FIXME: Be care when you will work with
		//large numbers
		parr = arr->ptr + arr->item_sz * i;
		pcopy = copy->ptr + copy->item_sz * i;
		f(pcopy, parr);
	}

	return copy;
}

//FIXME: be care when will introduce large numbers
ret_t
arr_set_item(arr_t *arr, int *ind, void *var)
{
	void **p;
	int i, mult, n;

	assert(arr != NULL && ind != NULL && var != NULL);

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

	*p = var;

	return ret_ok;
}

ret_t
arr_get_item(arr_t *arr, int *ind, void **pvar)
{
	void **p;
	int i, mult, n;

	assert(arr != NULL && ind != NULL && pvar != NULL
	    && *pvar != NULL);

	mult = 1;
	n = 0;

	for (i = arr->dims - 1; i >= 0; i--) {
		if (ind[i] >= arr->len[i] || ind[i] < 0)
			return ret_invalid;
		
		n += ind[i] * mult;
		mult *= arr->len[i];
	}
	
	p = arr->ptr + arr->item_sz * n;
	*pvar = *p;

	return ret_ok;
}

void
arr_print(arr_t *arr, arr_item_print_t fprint)
{
	int i, n;
	void *var;
	int *index;

	assert(arr != NULL);

#if IS_DEBUG == 1
	
	DEBUG(LOG_DEFAULT, "%d dims\n", arr->dims);
 	for (i = 0; i < arr->dims; i++)
		DEBUG(LOG_DEFAULT, "%d) len = %d\n", i, arr->len[i]);
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
			arr_get_item(arr, index, &var);
			fprint(var);
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
arr_free(arr_t *arr, arr_item_destructor_t f)
{
	int i;

	for (i = 0; i < arr->sz; i++) {
		f(arr->ptr[i]);
		ufree(arr->ptr[i]);
	}
	
	ufree(arr->len);
	ufree(arr->ptr);
	ufree(arr);
}


