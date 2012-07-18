#include <string.h>

#include "function.h"
#include "hash.h"
#include "id_table.h"
#include "macros.h"
#include "syn_tree.h"

#define INITIAL_SZ 32

static struct hash_table *func_table;

// need to imlement builtin functions
//struct func_t builtin[];

static void function_table_fill();

static int
function_compare(void *a, void *b)
{	
	return strcmp((char*)a, (char*)b);
}

static unsigned long
function_hash_cb(const void *data)
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

//FIXME destroy list
static void
function_table_destroy_cb(func_t *item)
{
	ufree(item->name);
	
	//free list
	if (item->is_lib == FALSE) {
		id_table_free(item->id_table);
		ast_node_unref(item->body);
	}

	ufree(item);
}

void 
function_table_init()
{
	if (func_table != NULL)
		print_warn_and_die("function already initialisated!");
	
	func_table = hash_table_new(INITIAL_SZ, 
	    (hash_callback_t )function_hash_cb,
	    (hash_compare_t )function_compare);
	
	if (func_table == NULL)
		print_warn_and_die("error at table table creation\n");
	
	//function_table_fill();
}

/*
// FIXME
static void
function_table_fill()
{
	int i;
	func_t *tmp;

	if (func_table == NULL)
		print_warn_and_die("func_table uninit\n");
	
	for (i = 0; i < ARRSZ(functions); i++) {
		tmp = malloc_or_die(sizeof(*tmp));
		
		//fill table with built in functions

		function_table_insert(tmp);
	}
}
*/

ret_t 
function_table_insert(func_t *item)
{
	int res;
	
	res = hash_table_insert_unique(func_table, item->name, item);
	if (res == ret_out_of_memory)
		print_warn_and_die("error at func table insertion\n");
	else if (res == ret_entry_exists)
		print_warn_and_die("internal error, entry exists\n");

	return ret_ok;
}

func_t *
function_table_lookup(char *name)
{
	func_t *res;

	if (hash_table_lookup(func_table, name, (void **)&res) != ret_ok)
		return NULL;
	else
		return res;
}

void 
function_table_destroy()
{
	void *key, *data;
	struct hash_table_iter *iter;

	iter = hash_table_iterate_init(func_table);
	
	while (hash_table_iterate(iter, &key, &data) != FALSE)
		function_table_destroy_cb((func_t*)data);

	hash_table_iterate_deinit(&iter);

	hash_table_destroy(&func_table);
}

/////////////////////////

func_t *
func_new()
{
	func_t *func;

	func = malloc_or_die(sizeof(*func));

	memset(func, 0, sizeof(*func));

	return func;
}

ret_t
func_delete(func_t *func)
{
	int ret;

	if (func->is_lib)
		return ret_err;
	
	ret = hash_table_remove(func_table, func->name);
	if (ret == FALSE)
		return ret_err;
	
	function_table_destroy_cb(func);

	return ret_ok;
}

void
func_add_argu(func_t *func, char *name)
{

}

void
func_set_body(func_t *func)
{


}
