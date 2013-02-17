#ifndef ARRAY_H_
#define ARRAY_H_

#include "common.h"

//array fields separator default is \034
extern char *arr_sep;

typedef struct {
	struct hash_table *hash;
} arr_t;

typedef struct {
	char *key;
	struct variable *var;
} arr_item_t;


/* 
 * Create new empty array
 */
arr_t *arr_new();

/*
 * Copy array with all elements
 */
arr_t *arr_copy(arr_t *arr);

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

#endif

