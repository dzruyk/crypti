#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "hash.h"
#include "primes.h"
#include "common.h"

#define HASH_SIZE	31

#define COLLISION_RATE(t)	((float)(t)->count / (float)(t)->collision)

#define WARN(fmt, arg...) \
do { \
	warning("%s: " fmt, __FUNCTION__, ##arg); \
} while (0);

struct hash_table {
	struct hash_bucket *arr;
	size_t		size;
	hash_callback_t	hash_cb;
	hash_compare_t	cmpl_cb;
	size_t		count;
	size_t		collision;
};

struct hash_bucket {
	struct hash_bucket *next;
	void		*key;
	void		*data;
};

struct hash_table_iter {
	struct hash_table *table;
	struct hash_bucket *bucket;
	unsigned int	idx;
};

static unsigned long
default_hash_cb(const void *ptr)
{
	return (unsigned long) ptr;
}

static int
default_key_cmpl_cb(const void *a, const void *b)
{
	if (a > b)
		return 1;
	else if (a < b)
		return -1;
	return 0;
}

struct hash_table *
hash_table_new(size_t hint_size, hash_callback_t hash, hash_compare_t key_cmp)
{
	struct hash_table *table;

	table = malloc(sizeof(struct hash_table));

	if (table == NULL) {
		WARN("can't allocate table");
		return NULL;
	}

	memset(table, 0, sizeof(struct hash_table));

	table->size = (hint_size == 0) ? HASH_SIZE : prime_nearest(hint_size);
	table->hash_cb = (hash == NULL) ? default_hash_cb : hash;
	table->cmpl_cb = (key_cmp == NULL) ? default_key_cmpl_cb : key_cmp;
	table->collision = 0;
	table->count = 0;

	table->arr = malloc(table->size * sizeof(struct hash_bucket));

	if (table->arr == NULL) {
		WARN("can't allocate array");
		free(table);
		return NULL;
	}

	memset(table->arr, 0, table->size * sizeof(struct hash_bucket));

	return table;
}

void
hash_table_destroy(struct hash_table **table)
{
	struct hash_bucket *bp, *next;
	unsigned int idx;

	return_if_fail(table != NULL);
	return_if_fail(*table != NULL);

	for (idx = 0; idx < (*table)->size; idx++) {

		if ((*table)->arr[idx].next == NULL)
			continue;

		for (bp = (*table)->arr[idx].next; bp != NULL; bp = next) {
			next = bp->next;
			free(bp);
		}
	}

	free((*table)->arr);
	free(*table);
	(*table) = NULL;
}

static ret_t
_hash_table_insert(struct hash_table *table, void *key, void *data)
{
	struct hash_bucket *bp;
	struct hash_bucket *arr;
	unsigned int idx, new_idx;
	unsigned int new_size;
	unsigned int collision = 0;

	if (COLLISION_RATE(table) <= 2.0) {

		new_size = prime_nearest(table->size+1);
		arr = malloc(new_size * sizeof(struct hash_bucket));

		if (arr == NULL) {
			WARN("can't allocate array");
			return ret_out_of_memory;
		}

		memset(arr, 0, new_size * sizeof(struct hash_bucket));

		/* Repopulate the new hash with data from the old one. */
		for (idx = 0; idx < table->size; idx++) {
			struct hash_bucket *next;

			if (table->arr[idx].key == NULL)
				continue;

			for (bp = &table->arr[idx]; bp != NULL; bp = next) {

				next = bp->next;
				new_idx = table->hash_cb(bp->key) % new_size;

				if (arr[new_idx].key == NULL) {
					arr[new_idx].key = bp->key;
					arr[new_idx].data = bp->data;
					arr[new_idx].next = NULL;
					if (bp != &table->arr[idx])
						free(bp);
				} else {
					if (bp == &table->arr[idx]) {
						struct hash_bucket *nbp;

						nbp = malloc(sizeof(struct hash_bucket));
						if (nbp == NULL){
							WARN("can't allocate bucket");
							return ret_out_of_memory;

						}
						memset(nbp, 0, sizeof(struct hash_bucket));
						nbp->key = bp->key;
						nbp->data = bp->data;
						/* forget about original bp */
						bp = nbp;
					}
					bp->next = arr[new_idx].next;
					arr[new_idx].next = bp;

					collision++;
				}
			}
		}

		/* Say goodbye to the old hash table. */
		free(table->arr);
		table->arr = arr;
		table->size = new_size;
		table->collision = collision;
	}

	idx = table->hash_cb(key) % table->size;

	if (table->arr[idx].key == NULL) {
		table->arr[idx].key = key;
		table->arr[idx].data = data;
		table->count++;
	} else {
		bp = malloc(sizeof(struct hash_bucket));
		if (bp == NULL){
			WARN("can't allocate bucket");
			return ret_out_of_memory;
		}
		memset(bp, 0, sizeof(struct hash_bucket));

		bp->key = key;
		bp->data = data;

		bp->next = table->arr[idx].next;
		table->arr[idx].next = bp;
		table->collision++;
		table->count++;
	}

	return ret_ok;
}

ret_t
hash_table_insert(struct hash_table *table, void *key, void *data)
{
	return_val_if_fail(table != NULL, -1);
	return_val_if_fail(key != NULL, -1);

	return _hash_table_insert(table, key, data);
}

ret_t
hash_table_insert_unique(struct hash_table *table, void *key, void *data)
{
	struct hash_bucket *nb;
	unsigned int idx;

	return_val_if_fail(table != NULL, FALSE);
	return_val_if_fail(key != NULL, FALSE);

	idx = table->hash_cb(key) % table->size;

	if (table->arr[idx].key != NULL) {
		for (nb = &table->arr[idx]; nb != NULL; nb = nb->next) {
			if (table->cmpl_cb(nb->key, key) == 0)
				return ret_entry_exists;
		}
	}

	return _hash_table_insert(table, key, data);
}

boolean_t
hash_table_remove(struct hash_table *table, void *key)
{
	struct hash_bucket *nb;
	unsigned int idx;

	return_val_if_fail(table != NULL, FALSE);
	return_val_if_fail(key != NULL, FALSE);

	idx = table->hash_cb(key) % table->size;

	if (table->arr[idx].key == NULL)
		return FALSE;

	if (table->cmpl_cb(table->arr[idx].key, key) == 0) {

		if (table->arr[idx].next != NULL) {
			nb = table->arr[idx].next;
			table->arr[idx].key = nb->key;
			table->arr[idx].data = nb->data;
			table->arr[idx].next = nb->next;
			free(nb);
			table->collision--;
		} else {
			table->arr[idx].key = NULL;
			table->arr[idx].data = NULL;
		}

		table->count--;
		return TRUE;

	} else if (table->arr[idx].next != NULL){

		struct hash_bucket *prev = NULL;

		for (nb = table->arr[idx].next; nb != NULL; nb = nb->next) {
			if (table->cmpl_cb(nb->key, key) == 0) {

				if (prev != NULL)
					prev->next = nb->next;
				else
					table->arr[idx].next = nb->next;

				table->collision--;
				table->count--;
				free(nb);
				return TRUE;
			}
			prev = nb;
		}
	}

	return FALSE;
}

void
hash_table_clean(struct hash_table *table)
{
	unsigned int idx;
	struct hash_bucket *nb, *next;

	return_if_fail(table != NULL);

	for (idx = 0; idx < table->size; idx++) {

		if (table->arr[idx].key == NULL)
			continue;

		if (table->arr[idx].next != NULL) {
			for (nb = table->arr[idx].next; nb != NULL; nb = next) {
				next = nb->next;
				free(nb);
			}
		}

		memset(&table->arr[idx], 0, sizeof(struct hash_bucket));
	}

	table->count = 0;
	table->collision = 0;
}

boolean_t
hash_table_replace(struct hash_table *table, void *key, void *data)
{
	struct hash_bucket *nb;
	unsigned int idx;

	return_val_if_fail(table != NULL, FALSE);
	return_val_if_fail(key != NULL, FALSE);

	idx = table->hash_cb(key) % table->size;

	if (table->arr[idx].key == NULL)
		return FALSE;

	for (nb = &table->arr[idx]; nb != NULL; nb = nb->next) {
		if (table->cmpl_cb(nb->key, key) == 0) {
			nb->data = data;
			return TRUE;
		}
	}

	return FALSE;
}

ret_t
hash_table_lookup(struct hash_table *table, void *key, void **res_data)
{
	struct hash_bucket *nb;
	unsigned int idx = 0;

	return_val_if_fail(table != NULL, -1);
	return_val_if_fail(key != NULL, -1);

	idx = table->hash_cb(key) % table->size;

	if (table->arr[idx].key == NULL)
		return ret_not_found;

	for (nb = &table->arr[idx]; nb != NULL; nb = nb->next) {
		if (table->cmpl_cb(nb->key, key) == 0) {
			if (res_data != NULL)
				*res_data = nb->data;
			return ret_ok;
		}
	}

	return ret_err;
}

struct hash_table_iter *
hash_table_iterate_init(struct hash_table *table)
{
	struct hash_table_iter *iter;

	return_val_if_fail(table != NULL, NULL);

	iter = malloc(sizeof(struct hash_table_iter));

	if (iter == NULL) {
		WARN("can't allocate iterator");
		return NULL;
	}

	iter->table = table;
	iter->idx = 0;
	iter->bucket = NULL;

	return iter;
}

void
hash_table_iterate_deinit(struct hash_table_iter **iter)
{
	return_if_fail(iter != NULL);
	return_if_fail(*iter != NULL);

	FREE(*iter);
}

boolean_t
hash_table_iterate(struct hash_table_iter *iter, void **res_key, void **res_data)
{
	struct hash_table *table;

	return_val_if_fail(iter != NULL, FALSE);
	return_val_if_fail(iter->table != NULL, FALSE);

	table = iter->table;

	if (iter->bucket == NULL) {
		/* Skip to next bucket. */
		while (iter->idx < table->size) {
			if (table->arr[iter->idx].key != NULL) {
				iter->bucket = &(table->arr[iter->idx]);
				break;
			}
			iter->idx++;
		}
	}

	if (iter->bucket != NULL) {

		if (res_key != NULL)
			*res_key = iter->bucket->key;
		if (res_data != NULL)
			*res_data = iter->bucket->data;

		if (iter->bucket->next != NULL) {
			iter->bucket = iter->bucket->next;
		} else {
			iter->bucket = NULL;
			iter->idx++;
		}

		return TRUE;
	}

	return FALSE;
}
