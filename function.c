#include <assert.h>
#include <string.h>

#include <stdio.h>

#include "function.h"
#include "hash.h"
#include "libcall.h"
#include "macros.h"

#define INITIAL_SZ 32

static struct hash_table *func_table;

// need to imlement builtin functions


static struct {
	char *name;
	int nargs;
	libcall_handler_t handler;
} builtin [] = {
	{"print", 1, libcall_print},
	{"sum", -1, libcall_sum},
	{"del", 1, libcall_del},
	{"type", 1, libcall_type},
};


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

static void
function_table_destroy_cb(func_t *item)
{
	
	//FIXME: Library func
	
	if (item->is_lib == FALSE) {
		int i;
		
		ufree(item->name);
		

		//FIXME: need to write non recursion finalize?
		/*
		ast_node_t *tmp, *prev;

		for (tmp = prev = item->body; tmp != NULL; prev = tmp) {
			tmp = prev->child;
			ast_node_unref(prev);
		}
		*/
		ast_node_unref(item->body);

		for (i = 0; i < item->nargs; i++)
			ufree(item->args[i]);
		ufree(item->args);
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
	
	function_table_fill();
}


// FIXME
static void
function_table_fill()
{
	int i;
	func_t *tmp;

	if (func_table == NULL)
		print_warn_and_die("func_table uninit\n");
	
	for (i = 0; i < ARRSZ(builtin); i++) {
		tmp = malloc_or_die(sizeof(*tmp));
		
		//fill table with built in functions
		tmp->is_lib = 1;
		tmp->name = builtin[i].name;
		tmp->handler = builtin[i].handler;
		tmp->nargs = builtin[i].nargs;

		function_table_insert(tmp);
	}
}


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
func_new(char *name)
{
	func_t *func;

	func = malloc_or_die(sizeof(*func));

	memset(func, 0, sizeof(*func));

	func->name = strdup_or_die(name);
	func->is_lib = 0;

	return func;
}

ret_t
func_table_delete(func_t *func)
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

//set args and add id_items to scope
void
func_set_args(func_t *func, char **args, int nargs)
{
	assert(func != NULL);

	int i;
	
	if (nargs > 0)
		func->args = malloc_or_die(sizeof(*args) * nargs);

	for (i = 0; i < nargs; i++)
		func->args[i] = strdup_or_die(args[i]);

	func->nargs = nargs;
}

void
func_set_body(func_t *func, void *body)
{
	assert(func != NULL);

	func->body = body;
}

