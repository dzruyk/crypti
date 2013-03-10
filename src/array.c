#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "log.h"
#include "hash.h"
#include "macros.h"
#include "variable.h"

static int initial_size = 32;

char *arr_sep = "\034";

static unsigned long
arr_hash_cb(const void *data)
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

static int
arr_cmp(const void *a, const void *b)
{       
	return strcmp((char*)a, (char*)b);
}

static arr_item_t *
arr_item_new(char *key, struct variable *var)
{
	arr_item_t *item;
	struct variable *tmp;

	item = xmalloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	
	tmp = xmalloc(sizeof(*tmp));
	var_init(tmp);
	var_copy(tmp, var);

	item->key = strdup_or_die(key);
	item->var = tmp;

	return item;
}

static void
arr_item_free(arr_item_t *item)
{
	ufree(item->key);
	var_clear(item->var);
	ufree(item->var);
	ufree(item);
}

arr_t *
arr_new()
{
	arr_t *arr;

	arr = xmalloc(sizeof(*arr));
	
	arr->hash = hash_table_new(initial_size, (hash_callback_t) arr_hash_cb,
	    (hash_compare_t) arr_cmp);

	return arr;
}

arr_t *
arr_dup(arr_t *arr)
{
	arr_t *copy;
	struct hash_table_iter *iter;
	arr_item_t *item;
	char *key;

	assert(arr != NULL);

	copy = arr_new();

	iter = hash_table_iterate_init(arr->hash);
	if (iter == NULL)
		error(1, "hash_table_iterate_init fail\n");

	while (hash_table_iterate(iter, (void **)&key, (void **)&item) != FALSE) {
		arr_set_item(copy, key, item->var);
	}

	hash_table_iterate_deinit(&iter);

	return copy;
}

//FIXME: be care when will introduce large numbers
void
arr_set_item(arr_t *arr, char *key, struct variable *var)
{
	arr_item_t *item;
	int ret;

	assert(arr != NULL && key != NULL && var != NULL);

	DEBUG(LOG_VERBOSE, "try to set item for key = %s\n", key);

	if (arr_remove_item(arr, key) != ret_ok)
		DEBUG(LOG_VERBOSE, "unsuccessfull item remove\n");

	item = arr_item_new(key, var);

	ret = hash_table_insert(arr->hash, item->key, item);
	if (ret == ret_out_of_memory)
		error(1, "error at array insertion\n");
	else if (ret != ret_ok)
		error(1, "internal error, should not happen\n");
}

struct variable *
arr_get_item(arr_t *arr, char *key)
{
	arr_item_t *item;

	assert(arr != NULL && key != NULL);

	if (hash_table_lookup(arr->hash, key, (void **)&item) != ret_ok)
		return NULL;

	return item->var;
}

ret_t
arr_remove_item(arr_t *arr, char *key)
{
	arr_item_t *old;

	if (hash_table_lookup(arr->hash, key, (void **)&old) != ret_ok)
		return ret_err;

	if (hash_table_remove(arr->hash, key) == FALSE)
		return ret_err;

	arr_item_free(old);

	return ret_ok;
}

void
arr_print(arr_t *arr)
{
	struct hash_table_iter *iter;
	struct variable *var;
	arr_item_t *item;
	str_t *str;
	void *key;

	assert(arr != NULL);
	
	printf("{");

	iter = hash_table_iterate_init(arr->hash);
	if (iter == NULL)
		error(1, "hash_table_iterate_init fail\n");

	if (hash_table_iterate(iter, &key, (void **)&item) == FALSE)
		item = NULL;

	while (item != NULL) {
		var = item->var;
		str = var_cast_to_str(var);
		printf("%s", str_ptr(str));
		if (hash_table_iterate(iter, &key, (void **)&item) == FALSE)
			item = NULL;
		else
			printf(", ");

	}

	printf("}\n");
	hash_table_iterate_deinit(&iter);
}


void
arr_free(arr_t *arr)
{
	struct hash_table_iter *iter;
	arr_item_t *item;
	void *key;

	iter = hash_table_iterate_init(arr->hash);
	if (iter == NULL)
		error(1, "hash_table_iterate_init fail\n");

	while (hash_table_iterate(iter, &key, (void **)&item) != FALSE) {
		arr_item_free(item);
	}

	hash_table_iterate_deinit(&iter);
	hash_table_destroy(&arr->hash);
	ufree(arr);
}

