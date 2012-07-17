#ifndef __ID_TABLE_H__
#define __ID_TABLE_H__

#include "array.h"
#include "common.h"
#include "function.h"

typedef enum {
	ID_NUM,
	ID_ARR,
	ID_FUNC,
	ID_UNKNOWN,
} id_type_t;

typedef struct {
	id_type_t type;
	char *name;
	union {
		int value;
		func_t *func;
		arr_t *arr;
	};
} id_item_t;

/*
 * initiates identifier tables
 * since we need more than one scope
 * we need to create stack of scopes.
 */
void id_table_init();

/*
 * add new identifier table
 */
struct hash_table * id_table_create();

/*
 * try to insert item into current scope 
 * or die if out of range or entry exists
 */
ret_t id_table_insert(id_item_t *item);

/*
 * lookup item with passed name and returns
 * pointer to item if finds
 * NULL if not finds
 */
id_item_t *id_table_lookup(char *name);

/*
 * free current table
 */
void id_table_free(struct hash_table *table);
/*
 * destroy all scopes
 */
void id_table_destroy();

#endif

