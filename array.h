#ifndef ARRAY_H_
#define ARRAY_H_

#include "common.h"

typedef struct {
	int n;
	int item_sz;
	void *ptr;
} arr_t;

/* 
 * Create new array with given size and item_sz, and return pointer to it 
 */
arr_t *arr_new(int n, int item_sz);

/* 
 * Sets value of arr[ind]
 * returns:
 * ret_ok if all ok
 * ret_invalid if ind out of range
 */
ret_t arr_set_item(arr_t *arr, int ind, int value);

/* 
 * set value with content of arr[ind]
 * returns:
 * ret_ok if all ok
 * ret_invalid if ind out of range
 */
ret_t arr_get_item(arr_t *arr, int ind, int *value);

/* 
 * Free memory allocated by arr and arr->ptr
 * WARN: dont set arr to NULL
 */
void arr_free(arr_t *arr);

/* 
 * Print array 
 * debug function
 * later need to rewrite or replace
 */
void arr_print(arr_t *arr);

#endif

