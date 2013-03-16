#ifndef ARRAY_H_
#define ARRAY_H_

#include "common.h"

//array fields separator default is \034
extern char *arr_sep;

typedef struct {
	struct hash_table *hash;
	int nitems;
} arr_t;

typedef struct {
	char *key;
	struct variable *var;
} arr_item_t;

typedef struct {
	struct hash_table_iter *iter;
} arr_iterate_t;
/* 
 * Create new empty array
 */
arr_t *arr_new();

/*
 * Copy array with all elements
 */
arr_t *arr_dup(arr_t *arr);

/* 
 * Sets value of arr[key]
 */
void arr_set_item(arr_t *arr, char *key, struct variable *var);

/* 
 * set value with content of arr[key]
 * returns:
 * arr_item if all OK
 * NULL if fails
 */
struct variable *arr_get_item(arr_t *arr, char *key);

/*
 * remove element by key from array.
 * returns:
 * ret_ok if suceeds
 * ret_err if error occured
 */
ret_t arr_remove_item(arr_t *arr, char *key);

/*
 * print passed array
 */
void arr_print(arr_t *arr);

/* 
 * Free memory allocated by arr and arr->ptr
 * WARN: dont set arr to NULL
 */
void arr_free(arr_t *arr);

/*
 * Functions for array elements iteration
 */
arr_iterate_t *array_iterate_new(arr_t *arr);

boolean_t array_iterate(arr_iterate_t *iterate, arr_item_t **res);

void array_iterate_free(arr_iterate_t *arr);

#endif

