#ifndef HASH_H_
#define HASH_H_

#include "common.h"

struct hash_table;
struct hash_table_iter;

typedef unsigned long (*hash_callback_t)(const void *data);
typedef int (*hash_compare_t)(const void *a, const void *b);

/* hash_table_new - creates new hash table. 
 * hint_size: hint for initial table size selection.
 * hash: pointer to hash function implementation (optional) or NULL.
 * key_cmp: pointer to key compare function (optional) or NULL.
 *
 * Returns: hash_table or NULL on error.
 */
struct hash_table *hash_table_new(size_t hint_size, hash_callback_t hash, hash_compare_t key_cmp);

/* hash_table_destroy - destroys table releasing it's resources.
 *
 * table: pointer holding pointer to hash table.
 */
void hash_table_destroy(struct hash_table **table);

/* hash_table_insert - inserts new key and data pairs.
 *
 * table: hash table.
 * key: pointer to key.
 * data: pointer to data.
 *
 * Returns:
 *	ret_ok
 *	ret_out_of_memory
 */
ret_t hash_table_insert(struct hash_table *table, void *key, void *data);

/* hash_table_insert_unique: inserts unique key and data pair.
 *
 * table: hash table.
 * key: pointer to key.
 * data: pointer to data.
 *
 * Returns:
 *	ret_ok
 *	ret_entry_exists
 */
ret_t hash_table_insert_unique(struct hash_table *table, void *key, void *data);

/* hash_table_remove: removes key from table.
 *
 * table: hash table.
 * key: pointer to key.
 *
 * Returns:
 *	FALSE - this key is not present in the table.
 *	TRUE - success.
 */
boolean_t hash_table_remove(struct hash_table *table, void *key);

/* hash_table_replace - replaces key's data. 
 *
 * table: hash table.
 * key: pointer to key.
 * data: replacement data for key.
 *
 * Returns:
 *	ret_ok
 *	ret_not_found
 */
ret_t hash_table_replace(struct hash_table *table, void *key, void *data);

/* hash_table_lookup - looks up the key in the table.
 *
 * table: hash table.
 * key: pointer to key.
 * res_data: where to put key's data if entry found or NULL.
 *
 * Returns:
 *	ret_ok
 *	ret_not_found
 */
ret_t hash_table_lookup(struct hash_table *table, void *key, void **res_data);

/* hash_table_clean - remove all entries from the table.
 *
 * table: hash table.
 */
void hash_table_clean(struct hash_table *table);

/* hash_table_iterate_init - prepare hash table iterator.
 *
 * table: hash table.
 *
 * Returns: pointer to allocated iterator or NULL on error.
 */
struct hash_table_iter *hash_table_iterate_init(struct hash_table *table);

/* hash_table_iterate_deinit - finalize hash table iterator.
 *
 * iter: pointer to pointer to iterator returned by hash_table_iterate_init().
 */
void hash_table_iterate_deinit(struct hash_table_iter **iter);

/* hash_table_iterate - iterate through hash table key/value pairs.
 *
 * iter: hash table iterator returned by hash_table_iterate_init().
 * res_key: where to store entry's key or NULL.
 * res_data: where to store entry's data or NULL.
 *
 * Returns:
 *	FALSE - no entries left.
 *	TRUE - res_key and res_data contain entry's data.
 *
 * This function is designed to be usefull as a condition in a while
 * loop like in the following example:
 *
 * while (hash_table_iterate(iter, &key, &data)) {
 *	do something with key and data
 * }
 */
boolean_t hash_table_iterate(struct hash_table_iter *iter, void **res_key, void **res_data);


#endif /*HASH_H_*/
