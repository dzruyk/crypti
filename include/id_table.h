#ifndef __ID_TABLE_H__
#define __ID_TABLE_H__

#include "array.h"
#include "common.h"
#include "str.h"

typedef enum {
	ID_NUM,
	ID_ARR,
	ID_UNKNOWN,
} id_type_t;

struct id_item;

typedef void (*id_item_release_t)(struct id_item *item);

typedef struct id_item {
	id_type_t type;
	char *name;
	union {
		int value;
		str_t *str;
		arr_t *arr;
	};

	id_item_release_t destructor;
} id_item_t;

/*
 * create new id_item_t with specified name
 * and type ID_UNKNOWN
 */
id_item_t *id_item_new(char *name);

/*
 * set type and data of item
 * WARNING: you must manualy free previous setted values
 *
 */
void id_item_set(id_item_t *item, id_type_t type, void *data);

/*
 * free specified item
 */
void id_item_free(id_item_t *item);

/*
 * initiates identifier tables
 * since we need more than one scope
 * we need to create stack of scopes.
 */
void id_table_init();

/*
 * add new identifier table
 */
struct hash_table *id_table_create();

/*
 * push table to top scope(make them active)
 */
void id_table_push(struct hash_table *table);

/*
 * pop table from top scope(make previos table active)
 */
struct hash_table *id_table_pop();

/*
 * try to insert item into current scope 
 * or die if out of range or entry exists
 */
ret_t id_table_insert(id_item_t *item);

/*
 * try to insert item into hash_table *table
 * or die if out of range or entry exists
 */
ret_t id_table_insert_to(struct hash_table *table, id_item_t *item);

/*
 * lookup item with passed name 
 * in current scope and returns
 * pointer to item if finds
 * or NULL 
 */
id_item_t *id_table_lookup(char *name);

/*
 * lookup item with passed name
 * in given scope and returns pointer
 * to them or NULL
 */
id_item_t *id_table_lookup_in(struct hash_table *table, char *name);

/*
 * lookup item in all accessible scopes
 * return item_t * if finds
 * or NULL
 */
id_item_t *id_table_lookup_all(char *name);

/*
 * try to find item with "key" and remove it
 * returns:
 * ret_ok if success
 * ret_err otherwise
 */
ret_t id_table_remove(char *name);

/*
 * same as id_table_remove, but search in passed hash_table
 */
ret_t id_table_remove_from(struct hash_table *table, char *name);


/*
 * free current table
 */
void id_table_free(struct hash_table *table);

/*
 * destroy all scopes
 */
void id_table_destroy();

#ifdef DEBUG

void id_table_show_all_items();

#endif

#endif

