#include <assert.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "id_table.h"
#include "lex.h"
#include "macros.h"

#define INITIAL_SZ 32

struct scopes {
	struct scopes *prev;
	struct hash_table *scope;
};

struct scopes *scopes;

struct hash_table *global;
struct hash_table *current;


static int
id_compare(void *a, void *b)
{	
	return strcmp((char*)a, (char*)b);
}

static unsigned long
id_hash_cb(const void *data)
{
	int i, mult, res;
	char *s;
	
	mult = 31;
	res = 0;
	s = (char*)data;

	for (i = 0; i < strlen(data); i++)
		res = res * mult + s[i];
	return res;
}

/* ID_ITEM FUNCTIONS*/
id_item_t *
id_item_new(char *name)
{
	id_item_t * item;

	item = malloc_or_die(sizeof(*item));
	item->name = strdup_or_die(name);
	item->type = ID_UNKNOWN;

	return item;
}

void 
id_item_set(id_item_t *item, id_type_t type, void *data)
{
	item->type = type;

	switch (type) {
	case ID_NUM:
		//FIXME: something ubnormal
		item->value = *((int *)data);
		break;
	case ID_ARR:
		item->arr = (arr_t *) data;
		break;
	default:
		//now we cant set to item functions
		print_warn_and_die("unsupported type tryed to assing\n");
		break;
	}

}

void
id_item_free(id_item_t *item)
{
	free(item->name);
	free(item);
}


/* ID_TABLE FUNCTIONS */
void
id_table_init()
{
	if (scopes != NULL)
		print_warn_and_die("double id table initialisation\n");
	scopes = malloc_or_die(sizeof(*scopes));
	memset(scopes, 0, sizeof(*scopes));

	global = current = id_table_create();
	
	scopes->scope = global;
	scopes->prev = NULL;
}


struct hash_table *
id_table_create()
{
	struct hash_table *table;

	table = hash_table_new(INITIAL_SZ, (hash_callback_t)id_hash_cb,
	    (hash_compare_t)id_compare);
	
	if (table == NULL)
		print_warn_and_die("error at table table creation\n");
	
	return table;
}

void
id_table_push(struct hash_table *table)
{
	struct scopes *tmp;

	tmp = malloc_or_die(sizeof(*tmp));

	tmp->prev = scopes;
	tmp->scope = table;

	scopes = tmp;
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

	return table;
}

ret_t
id_table_insert_to(struct hash_table *table, id_item_t *item)
{
	assert(table != NULL);

	int res;
	
	res = hash_table_insert_unique(table, item->name, item);
	if (res == ret_out_of_memory)
		print_warn_and_die("error at id table insertion\n");
	else if (res == ret_entry_exists)
		print_warn_and_die("internal error, entry exists\n");

	return ret_ok;
}


ret_t
id_table_insert(id_item_t *item)
{
	return id_table_insert_to(current, item);
}


id_item_t *
id_table_lookup_in(struct hash_table *table, char *name)
{
	id_item_t *res;
	
	if (hash_table_lookup(table, name, (void**)&res) != ret_ok)
		return NULL;
	else
		return res;
}


id_item_t *
id_table_lookup(char *name)
{
	return id_table_lookup_in(current, name);
}


id_talbe_t *
id_table_lookup_all(char *name)
{
	id_item_t *item;
	struct scopes *tmp;
	
	tmp = scopes;
	while (tmp != NULL) {
		item = id_table_lookup_in(tmp->scope, name)
		if (item != NULL)
			return item;

		tmp = tmp->prev;
	}
	
	return NULL;
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

