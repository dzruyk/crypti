#include <assert.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "defaults.h"
#include "hash.h"
#include "id_table.h"
#include "lex.h"
#include "log.h"
#include "macros.h"
#include "variable.h"

struct scopes {
	struct scopes *prev;
	struct hash_table *scope;
};

struct scopes *scopes;

struct hash_table *global;
struct hash_table *current;

void
id_item_default_release(id_item_t *item)
{
	assert(item != NULL);

	switch (item->type) {
	case ID_VAR:
		if (item->var != NULL) {
			var_clear(item->var);
			ufree(item->var);
			item->var = NULL;
		}
		break;
	case ID_ARR:
		if (item->arr != NULL) {
			arr_free(item->arr);
			item->arr = NULL;
		}
		break;
	case ID_UNKNOWN:
	default:
		error(1, "unknown id type\n");
		break;
	}
}

/* ID_ITEM FUNCTIONS*/
id_item_t *
id_item_new(char *name)
{
	id_item_t * item;

	item = xmalloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	item->name = strdup_or_die(name);
	item->type = ID_UNKNOWN;
	item->destructor = id_item_default_release;

	return item;
}

void 
id_item_set(id_item_t *item, id_type_t type, void *data)
{
	//free previos variables
	if (item->type != ID_UNKNOWN)
		item->destructor(item);

	item->type = type;

	switch (type) {
	case ID_VAR:
		//FIXME: something ubnormal
		item->var = (struct variable *)data;
		break;
	case ID_ARR:
		item->arr = (arr_t *)data;
		break;
	default:
		//now we can't set to item functions
		error(1, "unsupported type tryed to assing\n");
		break;
	}

}

void
id_item_free(id_item_t *item)
{
	item->destructor(item);

	ufree(item->name);
	ufree(item);
}


/* ID_TABLE FUNCTIONS */
void
id_table_init()
{
	if (scopes != NULL)
		error(1, "double id table initialisation\n");
	scopes = xmalloc(sizeof(*scopes));
	memset(scopes, 0, sizeof(*scopes));

	global = current = id_table_create();
	
	scopes->scope = global;
	scopes->prev = NULL;
}


struct hash_table *
id_table_create()
{
	struct hash_table *table;

	table = hash_table_new(INITIAL_SZ, (hash_callback_t)default_hash_cb,
	    (hash_compare_t)default_hash_compare);
	
	if (table == NULL)
		error(1, "error at table table creation\n");
	
	return table;
}

void
id_table_push(struct hash_table *table)
{
	struct scopes *tmp;

	tmp = xmalloc(sizeof(*tmp));

	tmp->prev = scopes;
	tmp->scope = table;

	scopes = tmp;

	current = scopes->scope;
}

struct hash_table *
id_table_pop()
{
	assert(scopes != NULL);

	struct hash_table *table;
	struct scopes *tmp;
	
	tmp = scopes;

	scopes = tmp->prev;

	table = tmp->scope;
	free(tmp);

	current = scopes->scope;

	return table;
}

ret_t
id_table_insert_to(struct hash_table *table, id_item_t *item)
{
	assert(table != NULL);

	int res;
	
	res = hash_table_insert_unique(table, item->name, item);
	if (res == ret_out_of_memory)
		error(1, "error at id table insertion\n");
	else if (res == ret_entry_exists)
		error(1, "internal error, entry exists\n");
	else if (res != ret_ok)
		error(1, "internal error, should not happen\n");

	return ret_ok;
}


ret_t
id_table_insert(id_item_t *item)
{
	assert(item != NULL);

	return id_table_insert_to(current, item);
}


id_item_t *
id_table_lookup_in(struct hash_table *table, char *name)
{
	id_item_t *res;

	assert(table != NULL && name != NULL);
	
	if (hash_table_lookup(table, name, (void**)&res) != ret_ok)
		return NULL;
	else
		return res;
}


id_item_t *
id_table_lookup(char *name)
{
	assert(name != NULL);

	return id_table_lookup_in(current, name);
}


id_item_t *
id_table_lookup_all(char *name)
{
	id_item_t *item;
	struct scopes *tmp;
	
	assert(name != NULL);

	tmp = scopes;
	while (tmp != NULL) {
		item = id_table_lookup_in(tmp->scope, name);
		if (item != NULL)
			return item;

		tmp = tmp->prev;
	}
	
	return NULL;
}

ret_t
id_table_remove(char *name)
{
	struct scopes *tmp;
	ret_t ret;
	
	tmp = scopes;

	assert(name != NULL);

	while (tmp != NULL) {
		ret = id_table_remove_from(tmp->scope, name);
		if (ret == ret_ok)
			return ret_ok;

		tmp = tmp->prev;
	}
	return ret_err;

}


ret_t 
id_table_remove_from(struct hash_table *table, char *name)
{
	id_item_t *item;
	
	assert(table != NULL && name != NULL);

	item = id_table_lookup_in(table, name);
	if (item == NULL)
		return ret_err;

	//normaly never be triggered
	if (hash_table_remove(table, (void *)name) == FALSE)
		return ret_err;
	
	id_item_free(item);

	return ret_ok;
}


void
id_table_free(struct hash_table *table)
{
	void *key, *data;
	struct hash_table_iter *iter;

	iter = hash_table_iterate_init(table);
	
	while (hash_table_iterate(iter, &key, &data) != FALSE)
		id_item_free((id_item_t*)data);

	hash_table_iterate_deinit(&iter);

	hash_table_destroy(&table);
}

void
id_table_destroy()
{
	struct scopes *tmp;

	for (; scopes != NULL; scopes = tmp) {
		tmp = scopes->prev;
		id_table_free(scopes->scope);
		free(scopes);
	}
}

/* DEBUG */
#ifdef DEBUG
void
id_table_show_all_items()
{
	struct scopes *tmp;
	
	tmp = scopes;
	
	printf("getting info about id_items...\n");
	while (tmp != NULL) {
		id_item_t *item;
		struct hash_table_iter *iter;
		void *key;

		iter = hash_table_iterate_init(tmp->scope);
		
		while (hash_table_iterate(iter, &key, (void **)&item) == TRUE) {
			printf("name = %s, type = %d\n",
			    item->name, item->type);
		}

		hash_table_iterate_deinit(&iter);

		tmp = tmp->prev;
	}
	
	printf("finishing...\n");
}

#endif
