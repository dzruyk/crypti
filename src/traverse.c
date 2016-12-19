#include <assert.h>
#include <stdint.h>

#include <string.h>

#include "array.h"
#include "climits.h"
#include "common.h"
#include "eval.h"
#include "function.h"
#include "id_table.h"
#include "lex.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "stack.h"
#include "traverse.h"
#include "variable.h"
#include "var_op.h"

static void traverse(ast_node_t *tree);
void set_value_id(id_item_t *item, eval_t *ev);

typedef void (*traverse_cb)(ast_node_t *tree);

struct trav_ctx {
	int is_call;
	int is_cycle;

	int is_return;
	int is_break;
	int is_continue;

	func_t *func;
};

struct trav_ctx helper;

typedef enum {
	RES_ERROR,
	RES_OK,
	RES_BREAK,
	RES_CONTINUE,
	RES_RETURN,
} res_type_t;

res_type_t traverse_body(ast_node_t *tree);

static int nerrors = 0;

static void
traverse_var(ast_node_t *tree)
{
	eval_t *ev;
	struct variable *var, *copy;

	assert(tree != NULL);

	var = ((ast_node_var_t *)tree)->var;

	copy = xmalloc(sizeof(*copy));
	var_init(copy);
	var_copy(copy, var);

	ev = eval_var_new(copy);

	stack_push(ev);
}

static void
traverse_id(ast_node_t *tree)
{
	eval_t *ev;
	id_item_t *item;
	struct variable *var;
	arr_t *arr;
	char *name;

	assert(tree != NULL);

	name = ((ast_node_id_t *)tree)->name;

	item = id_table_lookup_all(name);
	if (item == NULL) {
		print_warn("symb %s undefined\n", name);
		nerrors++;
		return;
	}

	//FIXME: need to get copy of variable
	switch (item->type) {
	case ID_VAR:
		var = xmalloc(sizeof(*var));
		var_init(var);
		var_copy(var, item->var);
		ev = eval_var_new(var);
		break;
	case ID_ARR:
		arr = arr_dup(item->arr);
		ev = eval_arr_new(arr);
		break;
	default:
		error(1, "WIP\n");
	}

	stack_push(ev);
}

static void
traverse_arr(ast_node_t *tree)
{
	struct variable index;
	struct variable *res;
	arr_t *arr;
	eval_t *ev;
	ast_node_t **synarr;
	int i, sz;

	assert(tree != NULL);

	DEBUG(LOG_VERBOSE, "\n");

	synarr = ((ast_node_arr_t *)tree)->arr;
	sz = ((ast_node_arr_t *)tree)->sz;
	var_init(&index);

	//size of int
	arr = arr_new();

	for (i = 0; i < sz; i++) {
		str_t *str;
		char *s;

		traverse(synarr[i]);
		//error handle?
		if (nerrors > 0)
			goto finalize;

		if ((ev = stack_pop()) == NULL)
			error(1, "unexpected error\n");

		if (ev->type != EVAL_VAR)
			error(1, "can't set item\n");

		res = ev->var;

		var_set_int(&index, i);
		str = var_cast_to_str(&index);
		s = str_ptr(str);

		arr_set_item(arr, s, res);

		eval_free(ev);
	}

finalize:
	var_clear(&index);
	ev = eval_arr_new(arr);
	stack_push(ev);
}

/*
 * returns allocated index string
 * WARN: you must free it manually
 */
static char *
create_index(ast_node_t **ind, int dims)
{
	struct variable keyval;
	struct variable sep;
	eval_t *ev;
	char *key;
	int i;

	ev = NULL;
	key = NULL;
	var_initv(&keyval, &sep, NULL);
	var_set_str(&keyval, "");
	var_set_str(&sep, arr_sep);

	for (i = 0; i < dims; i++) {
		struct variable *var;

		traverse(ind[i]);

		if (nerrors != 0)
			goto err;

		ev = stack_pop();
		if (ev == NULL) {
			print_warn("can't get index\n");
			goto err;
		}
		if (ev->type != EVAL_VAR) {
			print_warn("index must be number\n");
			goto err;
		}
		var = ev->var;

		varop_str_concat(&keyval, &keyval, var);
		if (i < dims - 1)
			varop_str_concat(&keyval, &keyval, &sep);

		eval_free(ev);
	}
	key = str_ptr(var_str_ptr((&keyval)));
	key = strdup_or_die(key);

err:
	var_clearv(&keyval, &sep, NULL);

	return key;
}

static void
traverse_access(ast_node_t *tree)
{
	struct variable *num, *copy;
	ast_node_access_t *acc;
	eval_t *resev;
	id_item_t *item;
	char *name, *key;

	assert(tree != NULL);

	key = NULL;

	if (nerrors != 0)
		return;

	acc = (ast_node_access_t *)tree;

	name = acc->name;

	item = id_table_lookup_all(name);
	//FIXME
	if (item == NULL) {
		print_warn("symb %s undefined\n", name);
		goto error;
	}

	key = create_index(acc->ind, acc->dims);

	DEBUG(LOG_VERBOSE, "access to ind %s\n", key);

	if ((num = arr_get_item(item->arr, key)) == NULL) {
		print_warn("can't get item\n");
		nerrors++;
		goto error;
	}
	ufree(key);

	copy = xmalloc(sizeof(*copy));
	var_init(copy);
	var_copy(copy, num);

	resev = eval_var_new(copy);
	stack_push(resev);

	return;
error:
	ufree(key);

	nerrors++;
}

static void
traverse_func_def(ast_node_t *tree)
{
	func_t *func;
	ast_node_func_t *synfunc;
	int ret;

	assert(tree != NULL);

	synfunc = (ast_node_func_t *)tree;

	func = function_table_lookup(synfunc->name);
	if (func != NULL) {
		//WARNING: now we can't redefine builtin functions
		if (func->is_lib != 0) {
			print_warn("can't redefine builtin function\n");
			nerrors++;
			return;
		}

		ret = func_table_delete(func);
		if (ret != ret_ok)
			error(1, "cant delete from func table");
	}

	func = func_new(synfunc->name);

	func_set_retargs(func, synfunc->retargs,synfunc->nret);
	func_set_args(func, synfunc->args, synfunc->nargs);
	func_set_body(func, synfunc->body);

	function_table_insert(func);
}

boolean_t
push_return_args(func_t *func)
{
	id_item_t *items[MAXRETARGS];
	int i;

	assert(func != NULL);

	for (i = func->nret - 1; i >= 0; i--) {
		items[i] = id_table_lookup(func->retargs[i]);
		if (items[i] == NULL) {
			return FALSE;
		}
	}
	for (i = func->nret - 1; i >= 0; i--) {
		struct variable *var, *copy;
		arr_t *arr;
		eval_t *ev;

		/* copy & push finded items */
		switch (items[i]->type) {
		case ID_VAR:
			var = items[i]->var;

			copy = xmalloc(sizeof(*copy));
			var_init(copy);
			var_copy(copy, var);

			ev = eval_var_new(copy);
			break;
		case ID_ARR:
			arr = arr_dup(ev->arr);
			ev = eval_arr_new(arr);
			break;
		default:
			error(1, "unknown id_item_type");
		}
		stack_push(ev);
	}

	return TRUE;
}

void
exec_function(func_t *func)
{
	//FIXME: write me!
	ast_node_t *next;
	func_t *saved;
	res_type_t res;

	assert(func != NULL);

	next = func->body;

	saved = helper.func;
	helper.func = func;
	helper.is_call++;

	while (next != NULL) {
		res = traverse_body(next);
		switch (res) {
		case RES_ERROR:
			print_warn("traverse func err\n");
			return;
		case RES_CONTINUE:
			print_warn("unexpected continue\n");
			return;
		case RES_BREAK:
			print_warn("unexpected break\n");
			return;
		case RES_RETURN:
			helper.is_return--;
			goto finalize;
		default:
			next = next->child;
			continue;
		}
	}
	/* Return not found while processing body. */
	if (push_return_args(helper.func) == FALSE) {
		print_warn("Can't return all function arguments\n");
		nerrors++;
	}

finalize:

	helper.is_call--;
	helper.func = saved;
}

/*
 * Get next function argument.
 * If fails return errors and/or inc nerrors
 */
id_item_t *
get_next_argument(ast_node_t *argnode, char *hint)
{
	eval_t *ev;
	id_item_t *item;

	assert(argnode != NULL && hint != NULL);

	traverse(argnode);

	if (nerrors != 0)
		return NULL;

	ev = stack_pop();

	if (ev == NULL) {
		print_warn("cant get value\n");
		return NULL;
	}

	item = id_item_new(hint);

	set_value_id(item, ev);

	eval_free(ev);

	return item;
}

/*
 * Add args to func array, then
 * try to execute library function
 * WARN: we must be care with global args(passed by name)
 * bcs may be double free situation
 */
void
exec_library_function(func_t *func, ast_node_func_call_t *call)
{
	eval_t *ev;
	id_item_t **items;
	int err, i;
	int rtypes[MAXRETARGS];
	void *rvals[MAXRETARGS];

	items = NULL;

	if (call->nargs != 0) {
		/*
		 * NOTE: Last argument must be NULL (need for
		 *  variable length functions).
		 */
		items = xmalloc(sizeof(*items) * (call->nargs + 1));
		memset(items, 0, sizeof(*items) * (call->nargs + 1));
	}

	for (i = 0; i < call->nargs; i++) {
		//FIXME: now we use empty name to indicate
		//that variable didn't inserted to scope
		//may be more usefull to pass at libcall
		//new type?
		items[i] = get_next_argument(call->args[i], "");

		//FIXME: may be memory leak
		if (items[i] == NULL || nerrors != 0)
			goto finalize;
		}

	err = func->handler(items, (int *)rtypes, (void **)rvals);
	if (err != 0) {
		print_warn("error when exec %s\n", func->name);
		nerrors++;
		goto finalize;
	}

	for (i = func->nret - 1; i >= 0; i--) {
		switch (rtypes[i]) {
		case ID_VAR:
			ev = eval_var_new(rvals[i]);
			stack_push(ev);
			break;
		case ID_ARR:
			error(1, "WIP\n");
			break;
		case ID_UNKNOWN:
			DEBUG(LOG_DEFAULT, "return args isn't set\n");
			break;
		default:
			break;
		}
	}

finalize:

	//Destroy args
	for (i = 0; i < call->nargs; i++) {
		if (items[i] != NULL)
			id_item_free(items[i]);
	}

	ufree(items);
	return;
}

/* NOTE:
 * Don't remove thrid argument!
 * When we use recursion we must get args from
 * previous scope and push in new one, then we need to
 * call id_table_push(new scope).
 */
static void
add_args_to_scope(func_t *func, ast_node_func_call_t *call,
    struct hash_table *scope)
{
	int i;
	id_item_t *item;
	char *name;

	assert(func != NULL && func->args != NULL &&
	    func->nargs != FUNC_VAR_ARGS);

	for (i = 0; i < func->nargs; i++) {
		name = func->args[i];
		if (name == NULL)
			error(1, "NULL name ptr\n");

		item = get_next_argument(call->args[i], name);

		if (item == NULL)
			return;
		id_table_insert_to(scope, item);
	}
}

void
traverse_func_call(ast_node_t *tree)
{
	func_t *func;
	struct hash_table *idtable;
	ast_node_func_call_t *call;

	call = (ast_node_func_call_t *) tree;

	func = function_table_lookup(call->name);
	if (func == NULL) {
		nerrors++;
		print_warn("function undefined yet\n");
		return;
	}

	/*
	 * FIXME: now olny library functions can have var length
	 * parameters list
	 */
	if (func->nargs != FUNC_VAR_ARGS &&
	    call->nargs != func->nargs) {
		nerrors++;
		print_warn("function have %d paramethers\n", func->nargs);
		return;
	}

	if (func->is_lib)
		return exec_library_function(func, call);

	idtable = id_table_create();

	if (func->nargs != 0)
		add_args_to_scope(func, call, idtable);

	if (nerrors != 0)
		goto finalize;

	id_table_push(idtable);

	exec_function(func);

	id_table_pop();

finalize:
	id_table_free(idtable);
}

/*
 * Traverse tree and check trav_ctx structure.
 * Returns:
 * - RES_CONTINUE, RES_BREAK - if traversal break/continue
 * inside cycle(work with nesting);
 * - RES_RETURN - if traversal return inside function;
 * - RES_OK if no errors recognised and without
 * break/continue/return meeting;
 * - RES_ERROR otherwise;
 */
res_type_t
traverse_body(ast_node_t *tree)
{
	res_type_t res;

	assert(tree != NULL);

	//FIXME: mb need rewrite me?
	traverse(tree);

	if (nerrors != 0)
		return RES_ERROR;

	else if (helper.is_continue > 0) {
		if(helper.is_cycle > 0) {
			res = RES_CONTINUE;
		} else {
			res = RES_ERROR;
			helper.is_continue--;
		}
	} else if (helper.is_break > 0) {
		if (helper.is_cycle > 0) {
			res =  RES_BREAK;
		} else {
			res = RES_ERROR;
			helper.is_break--;
		}
	} else if (helper.is_return > 0) {
		if (helper.is_call > 0) {
			res = RES_RETURN;
		} else {
			res = RES_ERROR;
			helper.is_return--;
		}
	} else {
		res = RES_OK;
	}

	if (res == RES_ERROR)
		nerrors++;

	return res;
}

void
traverse_scope(ast_node_t *tree)
{
	ast_node_t *next;
	res_type_t res;

	assert(tree != NULL);

	//traverse next block
	//FIXME: work bad with multiple scopes in functions
	//or cycels
	next = tree->child;
	while (next != NULL) {
		res = traverse_body(next);
		switch (res) {
		case RES_ERROR:
		case RES_CONTINUE:
		case RES_BREAK:
		case RES_RETURN:
			return;
		default:
			next = next->child;
			continue;
		}
	}
}


/*
 * Traverse condition, check result,
 * inc error counter if need.
 * returns:
 * TRUE if condition true,
 * FALSE otherwise(condition false or errors occured)
 */
boolean_t
traverse_cond(ast_node_t *tree)
{
	eval_t *ev;
	boolean_t ret;

	assert(tree != NULL);

	traverse(tree);

	if (nerrors != 0)
		return FALSE;

	ev = stack_pop();
	if (ev == NULL) {
		print_warn("cant get operand\n");
		nerrors++;
		return FALSE;
	}

	if (eval_is_zero(ev) == TRUE)
		ret = FALSE;
	else
		ret = TRUE;

	eval_free(ev);

	return ret;
}

void
traverse_if(ast_node_t *tree)
{
	ast_node_if_t *ifnode;
	boolean_t ret;

	assert(tree != NULL);

	ifnode = (ast_node_if_t *) tree;

	ret = traverse_cond(ifnode->_if);

	if (nerrors != 0)
		return;

	if (ret == TRUE) {
		traverse(ifnode->body);
	} else if (ifnode->_else != NULL) {
		traverse(ifnode->_else);
	}
}

void
traverse_for(ast_node_t *tree)
{
	ast_node_for_t *fornode;
	res_type_t res;

	assert(tree != NULL);

	fornode = (ast_node_for_t *)tree;

	if (fornode->expr1 != NULL)
		traverse(fornode->expr1);

	if (nerrors != 0)
		return;

	helper.is_cycle++;

	while (1) {

		if (fornode->expr2 != NULL) {
			boolean_t ret;

			ret = traverse_cond(fornode->expr2);

			if (ret == FALSE)
				goto finalize;
		}

		res = traverse_body(fornode->body);

		switch (res) {
		case RES_ERROR:
			goto finalize;
		case RES_CONTINUE:
			helper.is_continue--;
			DEBUG(LOG_DEFAULT, "continue catch!\n");
			break;
		case RES_BREAK:
			helper.is_break--;
			DEBUG(LOG_DEFAULT, "break catch!\n");
			goto finalize;
		case RES_RETURN:
			goto finalize;
		default:
			break;
		}

		if (fornode->expr3 != NULL)
			traverse(fornode->expr3);

		if (nerrors != 0)
			return;
	}

finalize:
	helper.is_cycle--;
}

void
traverse_while(ast_node_t *tree)
{
	ast_node_while_t *whilenode;
	res_type_t res;

	assert(tree != NULL);

	whilenode = (ast_node_while_t *) tree;

	helper.is_cycle++;

	while (1) {
		boolean_t ret;

		ret = traverse_cond(whilenode->cond);

		if (ret == FALSE)
			goto finalize;

		if (whilenode->body == NULL)
			continue;

		res = traverse_body(whilenode->body);

		switch (res) {
		case RES_ERROR:
			goto finalize;
		case RES_CONTINUE:
			helper.is_continue--;
			DEBUG(LOG_DEFAULT, "continue catch!\n");
			continue;
		case RES_BREAK:
			helper.is_break--;
			DEBUG(LOG_DEFAULT, "break catch!\n");
			goto finalize;
		case RES_RETURN:
			goto finalize;
		default:
			break;
		}
		if (nerrors != 0)
			goto finalize;
	}

finalize:
	helper.is_cycle--;
}

void
traverse_do(ast_node_t *tree)
{
	ast_node_do_t *donode;
	res_type_t res;

	assert(tree != NULL);

	donode = (ast_node_do_t *) tree;

	helper.is_cycle++;

	while (1) {
		boolean_t ret;


		if (donode->body == NULL)
			goto check_condition;

		res = traverse_body(donode->body);

		switch (res) {
		case RES_ERROR:
			goto finalize;
		case RES_CONTINUE:
			helper.is_continue--;
			DEBUG(LOG_DEFAULT, "continue catch!\n");
			continue;
		case RES_BREAK:
			helper.is_break--;
			DEBUG(LOG_DEFAULT, "break catch!\n");
			goto finalize;
		case RES_RETURN:
			goto finalize;
		default:
			break;
		}
check_condition:
		ret = traverse_cond(donode->cond);

		if (ret == FALSE)
			goto finalize;

		if (nerrors != 0)
			goto finalize;
	}

finalize:
	helper.is_cycle--;
}

void
traverse_del(ast_node_t *tree)
{
	ast_node_del_t *delnode;
	ast_node_id_t *idnode;
	ast_node_access_t *acc;
	id_item_t *id;
	char *key;
	int ret;

	assert(tree != NULL);

	key = NULL;
	delnode = (ast_node_del_t *)tree;

	switch (delnode->id->type) {
	case AST_NODE_ID:
		idnode = (ast_node_id_t *)delnode->id;

		id = id_table_lookup_all(idnode->name);
		if (id == NULL) {
			print_warn("name %s is not defined\n", idnode->name);
			nerrors++;
			return;
		}

		ret = id_table_remove(idnode->name);
		if (ret != ret_ok) {
			print_warn("remove %s error\n", idnode->name);
			nerrors++;
			return;
		}

		break;
	case AST_NODE_ACCESS:
		acc = (ast_node_access_t *)delnode->id;

		id = id_table_lookup_all(acc->name);
		if (id == NULL) {
			print_warn("name %s is not defined\n", acc->name);
			nerrors++;
			return;
		}
		key = create_index(acc->ind, acc->dims);

		if (arr_remove_item(id->arr, key) != ret_ok) {
			print_warn("can't remove arr item\n");
			nerrors++;
			ufree(key);
			return;
		}
		ufree(key);

		break;
	default:
		SHOULDNT_REACH();
	}

	return;
}

void
set_value_id(id_item_t *item, eval_t *ev)
{
	struct variable *var;
	arr_t *arr;

	switch (ev->type) {
	case EVAL_VAR:
		var = xmalloc(sizeof(*var));
		var_init(var);
		var_copy(var, ev->var);

		id_item_set(item, ID_VAR, var);
		break;
	case EVAL_ARR:
		arr = arr_dup(ev->arr);

		id_item_set(item, ID_ARR, arr);
		break;
	default:
		error(1, "WIP\n");
	}
}


static void
set_value_node_access(ast_node_access_t *node, eval_t *newval)
{
	arr_t *arr;
	eval_t *ev;
	id_item_t *item;
	char *key;

	assert(newval != NULL);

	ev = NULL;
	key = NULL;

	if (newval->type != EVAL_VAR) {
		print_warn("try to assign not a number\n");
		goto err;
	}

	item = id_table_lookup_all(node->name);
	if (item == NULL) {
		item = id_item_new(node->name);
		arr = arr_new();
		id_item_set(item, ID_ARR, arr);
		id_table_insert(item);
	} else if (item->type == ID_ARR) {
		arr = item->arr;
	} else {
		print_warn("%s isn't array\n", item->name);
		goto err;
	}

	key = create_index(node->ind, node->dims);

	arr_set_item(arr, key, newval->var);

	ufree(key);

	return;
err:
	nerrors++;

	ufree(key);
	eval_free(ev);
}

static void
set_value_node(ast_node_t *ltree, eval_t *ev)
{
	ast_node_id_t *id;
	ast_node_access_t *acc;

	assert(ltree != NULL && ev != NULL);

	switch (ltree->type) {
	case AST_NODE_ID: {
		id_item_t *item;

		id = (ast_node_id_t *)ltree;

		//NOTE: quick fix
		item = id_table_lookup(id->name);
		//if we havent id, then we must define it
		if (item == NULL) {
			item = id_item_new(id->name);
			id_table_insert(item);
		}

		set_value_id(item, ev);

		break;}
	case AST_NODE_ACCESS:

		acc = (ast_node_access_t *)ltree;
		set_value_node_access(acc, ev);

		break;
	default:
		print_warn("assignment to not variable\n");
		nerrors++;
	}
}

static void
set_value_node_n(ast_node_t **tree, eval_t **evs, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		if (nerrors != 0)
			break;
		set_value_node(*tree++, *evs++);
	}
}


int
traverse_as_rest(ast_node_t *tree)
{
	ast_node_t *ltree;
	ast_node_seq_t *seq;
	eval_t **evs;
	int i;

	assert(tree != NULL);

	ltree = (ast_node_t *)tree->left;

	if (tree->right->type == AST_NODE_AS)
		traverse_as_rest(tree->right);
	else
		traverse(tree->right);

	if (nerrors != 0)
		return -1;

	if (ltree->type != AST_NODE_SEQ) {
		print_warn("lvalue not AST_NODE_SEQ, seems like crypti"
		    " developer mistake");
		nerrors++;
		return -1;
	}

	seq = (ast_node_seq_t *)ltree;
	evs = xmalloc(seq->n * sizeof(*evs));
	stack_pop_n((void **)evs, seq->n);

	for (i = 0; i < seq->n; i++) {
		if (evs[i] == NULL) {
			nerrors++;
			print_warn("cant get operand\n");
			ufree(evs);
			return -1;
		}
	}
	set_value_node_n(seq->node, evs, seq->n);

	if (nerrors != 0)
		goto finalize;

	//FIXME: simple workaround var
	//double free problem
	stack_push_n((void **)evs, seq->n);
finalize:
	ufree(evs);

	return seq->n;
}

//FIXME: now just simple fix problem
//with multiple assignment
// in top assignment we must pop last eval
static void
traverse_as(ast_node_t *tree)
{
	int n;

	assert(tree != NULL);

	n = traverse_as_rest(tree);

	if (nerrors != 0 || n < 0)
		return;

	stack_remove_n(n);
}

static void
traverse_seq(ast_node_t *tree)
{
	ast_node_seq_t *seq;
	int i;

	seq = (ast_node_seq_t *)tree;

	for (i = seq->n - 1; i >= 0; i--)
		traverse(seq->node[i]);
}

static void
traverse_trenary(ast_node_t *tree)
{
	ast_node_trenary_t *trenary;
	int ret;

	assert(tree != NULL);

	trenary = (ast_node_trenary_t *) tree;

	ret = traverse_cond(trenary->cond);
	if (nerrors != 0)
		return;
	if (ret == TRUE)
		traverse(trenary->_if_yes);
	else
		traverse(trenary->_if_no);

}

static void
traverse_unary(ast_node_t *tree)
{
	ast_node_unary_t *unary;
	eval_t *ev, *res;

	assert(tree != NULL);

	ev = res = NULL;

	unary = (ast_node_unary_t *) tree;

	traverse(unary->node);

	if (nerrors != 0)
		return;

	ev = stack_pop();
	if (ev == NULL) {
		nerrors++;
		print_warn("can't get operand\n");
		goto finalize;
	}

	res = eval_process_unary(ev, unary->opcode);

	if (res == NULL) {
		nerrors++;
		print_warn("operation error\n");
		goto finalize;
	}

	stack_push(res);

finalize:
	eval_free(ev);
}

static void
traverse_op(ast_node_t *tree)
{
	ast_node_op_t *optree;
	eval_t *left, *right, *res;

	assert(tree != NULL);

	traverse(tree->left);
	traverse(tree->right);

	optree = (ast_node_op_t *)tree;

	if (nerrors != 0)
		return;

	right = stack_pop();
	left = stack_pop();

	if (right == NULL || left == NULL) {
		nerrors++;
		print_warn("can't get operand\n");
		goto finalize;
	}

	res = eval_process_op(left, right, optree->opcode);

	if (res == NULL) {
		nerrors++;
		print_warn("operation error\n");
		goto finalize;
	}

	stack_push(res);

finalize:

	eval_free(left);
	eval_free(right);

}

static void
traverse_op_as(ast_node_t *tree)
{
	ast_node_op_as_t *optree;
	eval_t *left, *right, *res;

	assert(tree != NULL);

	traverse(tree->left);
	traverse(tree->right);

	optree = (ast_node_op_as_t *)tree;

	if (nerrors != 0)
		return;

	right = stack_pop();
	left = stack_pop();

	if (right == NULL || left == NULL) {
		nerrors++;
		print_warn("cant get operand\n");
		goto finalize;
	}

	res = eval_process_op(left, right, optree->opcode);
	if (res == NULL) {
		nerrors++;
		print_warn("operation error\n");
		goto finalize;
	}

	set_value_node(tree->left, res);

finalize:
	eval_free(res);
	eval_free(left);
	eval_free(right);
}

static void
traverse_return(ast_node_t *tree)
{
	assert(tree != NULL);

	helper.is_return++;

	if (push_return_args(helper.func) == FALSE) {
		print_warn("Can't return all function arguments\n");
		nerrors++;
	}
}

static void
traverse_import(ast_node_t *tree)
{
	ast_node_import_t *import;
	int i, old;

	assert(tree != NULL);

	import = (ast_node_import_t *) tree;

	old = nerrors;

	for (i = 0; i < import->len; i++) {
		nerrors = 0;
		traverse(import->nodes[i]);

		stack_flush();
	}

	nerrors = old;
}

static void
traverse_break(ast_node_t *tree)
{
	assert(tree != 0);

	helper.is_break++;
}

static void
traverse_continue(ast_node_t *tree)
{
	assert(tree != 0);

	helper.is_continue++;
}

static void
traverse_stub(ast_node_t *tree)
{
	assert(tree != 0);

	return;
}

struct {
	ast_type_t node;
	traverse_cb callback;
} node_type [] = {
	{AST_NODE_VAR, traverse_var},
	{AST_NODE_ID, traverse_id},
	{AST_NODE_ARR, traverse_arr},
	{AST_NODE_ACCESS, traverse_access},
	{AST_NODE_DEF, traverse_func_def},
	{AST_NODE_CALL, traverse_func_call},
	{AST_NODE_TRENARY, traverse_trenary},
	{AST_NODE_UNARY, traverse_unary},
	{AST_NODE_OP, traverse_op},
	{AST_NODE_OP_AS, traverse_op_as},
	{AST_NODE_AS, traverse_as},
	{AST_NODE_SEQ, traverse_seq},
	{AST_NODE_IF, traverse_if},
	{AST_NODE_FOR, traverse_for},
	{AST_NODE_WHILE, traverse_while},
	{AST_NODE_DO, traverse_do},
	{AST_NODE_DEL, traverse_del},
	{AST_NODE_BREAK, traverse_break},
	{AST_NODE_CONTINUE, traverse_continue},
	{AST_NODE_RETURN, traverse_return},
	{AST_NODE_IMPORT, traverse_import},
	{AST_NODE_SCOPE, traverse_scope},
	{AST_NODE_STUB, traverse_stub},
	{AST_NODE_UNKNOWN, NULL},
};

static void
traverse(ast_node_t *tree)
{
	int i;

	assert(tree != 0);

	for (i = 0; node_type[i].node != AST_NODE_UNKNOWN; i++)
		if (node_type[i].node == tree->type) {
			node_type[i].callback(tree);
			return;
		}
}

ret_t
traverse_prog(ast_node_t *tree)
{
	if (tree == NULL)
		return ret_ok;

	//clean vars
	nerrors = 0;
	memset(&helper, 0, sizeof(helper));

	traverse(tree);

	ast_node_unref(tree);

	if (nerrors != 0) {
		stack_flush();
		return ret_err;
	}

	return ret_ok;
}

void
traverse_result()
{
	eval_t *tmp;

	while ((tmp = (eval_t *)stack_pop()) != NULL) {
		eval_print_val(tmp);
		eval_free(tmp);
	}
}

