#ifndef ARRAY_H_
#define ARRAY_H_

#include "common.h"

typedef void (*arr_item_destructor_t)(void *item);
typedef void (*arr_item_copy_t)(void *dst, void *src);
typedef void (*arr_item_print_t)(void *item);

typedef struct {
	int dims;
	int *len;	//i'th dimention lenght
	int sz;		//total value of items in all dimentions
	int item_sz;
	void **ptr;
} arr_t;

/* 
 * Create new array with given size and item_sz, and return pointer to it 
 */
arr_t *arr_new(int dims, int *len, int sz, int item_sz);

/* 
 * Sets value of arr[ind]
 * returns:
 * ret_ok if all ok
 * ret_invalid if ind out of range
 */
ret_t arr_set_item(arr_t *arr, int *ind, void *var);

/* 
 * set value with content of arr[ind]
 * returns:
 * ret_ok if all ok
 * ret_invalid if ind out of range
 */
ret_t arr_get_item(arr_t *arr, int *ind, void **pvar);

/*
 * print passed array
 */
void arr_print(arr_t *arr, arr_item_print_t fprint);

/* 
 * Free memory allocated by arr and arr->ptr
 * WARN: dont set arr to NULL
 */
void arr_free(arr_t *arr, arr_item_destructor_t f);

#endif

