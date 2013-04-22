#include <assert.h>
#include <string.h>

#include <stdio.h>

#include "defaults.h"
#include "function.h"
#include "hash.h"
#include "libcall.h"
#include "macros.h"

static struct hash_table *func_table;

// need to imlement builtin functions

static struct {
	char *name;
	int nargs;
	int nret;
	libcall_handler_t handler;
} builtin [] = {
	{"print", 1, 0, libcall_print},
	{"printf", FUNC_VAR_ARGS, 0, libcall_printf},
	{"sum", FUNC_VAR_ARGS, 1, libcall_sum},
	/*{"del", 1, libcall_del},*/
	{"type", 1, 0, libcall_type},
	{"arr_min_max", 1, 2, libcall_arr_min_max},
	{"subs", 3, 1, libcall_subs},
	{"subocts", 3, 1, libcall_subocts},
	{"len", 1, 1, libcall_len},
	{"lpad", 2, 1, libcall_lpad},

	/* mpl function wrappers. */
	{"gcd", 2, 1, libcall_gcd},
	{"mod_inv", 2, 1, libcall_mod_inv},
	{"mod_exp", 3, 1, libcall_mod_exp},

	/* Crypto hashes (simple) */
	{"md5", 1, 1, libcall_md5},
	{"sha1", 1, 1, libcall_sha1},
	{"sha256", 1, 1, libcall_sha256},
	{"whirlpool", 1, 1, libcall_whirlpool},

	/* Crypto hashes (full) */
	{"md5_init", 1, 1, libcall_md5_init},
	{"md5_update", 2, 0, libcall_md5_update},
	{"md5_finalize", 1, 1, libcall_md5_finalize},
	{"sha1_init", 1, 1, libcall_sha1_init},
	{"sha1_update", 2, 0, libcall_sha1_update},
	{"sha1_finalize", 1, 1, libcall_sha1_finalize},
	{"sha256_init", 1, 1, libcall_sha256_init},
	{"sha256_update", 2, 0, libcall_sha256_update},
	{"sha256_finalize", 1, 1, libcall_sha256_finalize},
	{"whirlpool_init", 1, 1, libcall_whirlpool_init},
	{"whirlpool_update", 2, 0, libcall_whirlpool_update},
	{"whirlpool_finalize", 1, 1, libcall_whirlpool_finalize},
};


static void function_table_fill();

static void
function_table_destroy_cb(func_t *item)
{
	
	//FIXME: Library func
	
	if (item->is_lib == FALSE) {
		int i;
		
		ufree(item->name);
		
		ast_node_unref(item->body);
	
		for (i = 0; i < item->nret; i++)
			ufree(item->retargs[i]);
		ufree(item->retargs);
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
		error(1, "function table already initialisated!");
	
	func_table = hash_table_new(INITIAL_SZ, 
	    (hash_callback_t )default_hash_cb,
	    (hash_compare_t )default_hash_compare);
	
	if (func_table == NULL)
		error(1, "error at table table creation\n");
	
	function_table_fill();
}


// FIXME
static void
function_table_fill()
{
	int i;
	func_t *tmp;

	if (func_table == NULL)
		error(1, "func_table uninit\n");
	
	for (i = 0; i < ARRSZ(builtin); i++) {
		tmp = xmalloc(sizeof(*tmp));
		
		//fill table with built in functions
		tmp->is_lib = 1;
		tmp->name = builtin[i].name;
		tmp->handler = builtin[i].handler;
		tmp->nargs = builtin[i].nargs;
		tmp->nret = builtin[i].nret;

		function_table_insert(tmp);
	}
}


ret_t 
function_table_insert(func_t *item)
{
	int res;
	
	assert(item != NULL);

	res = hash_table_insert_unique(func_table, item->name, item);
	if (res == ret_out_of_memory)
		error(1, "error at func table insertion\n");
	else if (res == ret_entry_exists)
		error(1, "internal error, entry exists\n");

	return ret_ok;
}

func_t *
function_table_lookup(char *name)
{
	func_t *res;

	assert(name != NULL);

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
	
	assert(name != NULL);

	func = xmalloc(sizeof(*func));

	memset(func, 0, sizeof(*func));

	func->name = strdup_or_die(name);
	func->is_lib = 0;

	return func;
}

ret_t
func_table_delete(func_t *func)
{
	int ret;

	assert(func != NULL);

	if (func->is_lib)
		return ret_err;
	
	ret = hash_table_remove(func_table, func->name);
	if (ret == FALSE)
		return ret_err;
	
	function_table_destroy_cb(func);

	return ret_ok;
}

void
func_set_retargs(func_t *func, char **retargs, int nret)
{
	int i;
	
	assert(func != NULL);

	if (nret > 0)
		func->retargs = xmalloc(sizeof(*retargs) * nret);

	for (i = 0; i < nret; i++)
		func->retargs[i] = strdup_or_die(retargs[i]);

	func->nret = nret;
}


//set args and add id_items to scope
void
func_set_args(func_t *func, char **args, int nargs)
{
	int i;
	
	assert(func != NULL);

	if (nargs > 0)
		func->args = xmalloc(sizeof(*args) * nargs);

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

