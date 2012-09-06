#include <assert.h>

#include <string.h>

#include "array.h"
#include "crypti.h"
#include "common.h"
#include "function.h"
#include "eval.h"
#include "id_table.h"
#include "lex.h"
#include "list.h"
#include "macros.h"
#include "stack.h"

#include "traverse.h"

static void traverse(ast_node_t *tree);
void set_value_id(id_item_t *item, eval_t *ev);

typedef void (*traverse_cb)(ast_node_t *tree);

struct trav_ctx {
	int is_call;
	int is_cycle;
	
	int is_return;
	int is_break;
	int is_continue;
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
traverse_num(ast_node_t *tree)
{
	eval_t *ev;
	int val;
	
	val = ((ast_node_num_t *)tree)->num;
	
	ev = eval_num_new(val);

	stack_push(ev);
}

static void
traverse_id(ast_node_t *tree)
{
	eval_t *ev;
	char *name;
	id_item_t *item;

	name = ((ast_node_id_t *)tree)->name;

	item = id_table_lookup_all(name);
	if (item == NULL) {
		print_warn("symb %s undefined\n", name);
		nerrors++;
		return;
	}
	
	switch (item->type) {
	case ID_NUM:
		ev = eval_num_new(item->value);
		break;
	case ID_ARR:
		ev = eval_arr_new(item->arr);
		break;
	default:
		print_warn_and_die("WIP\n");
	}

	stack_push(ev);
}

/*
 * increment index array
 */
static void
get_next_index(int *index, int dims, int *len)
{
	int i;
	
	i = dims - 1;

	while (i >= 0) {
		index[i]++;
		if (index[i] >= len[i]) {
			index[i] = index[i] % len[i];
			i--;
			continue;
		} else {
			break;
		}
	}
}

static void
traverse_arr(ast_node_t *tree)
{
	arr_t *arr;
	eval_t *ev;
	ast_node_t **synarr;
	int i, dims, res, sz;
	int *len;
	int *index;
	
	synarr = ((ast_node_arr_t *)tree)->arr;
	len = ((ast_node_arr_t *)tree)->len;
	dims = ((ast_node_arr_t *)tree)->dims;
	sz = ((ast_node_arr_t *)tree)->sz;

	//size of int
	arr = arr_new(dims, len, sz, sizeof(int));

	index = xmalloc(dims * sizeof(*len));
	memset(index, 0, dims * sizeof(*len));
	
	
	for (i = 0; i < sz; i++) {
		traverse(synarr[i]);
		//error handle?
		if ((ev = stack_pop()) == NULL)
			print_warn_and_die("unexpected error\n");
		
		if (ev->type != EVAL_NUM)
			print_warn_and_die("cant set item\n");
		
		res = ev->value;

		if (arr_set_item(arr, index, res) != ret_ok)
			print_warn_and_die("cant set item\n");

		get_next_index(index, dims, len);

		eval_free(ev);
	}
	
	ev = eval_arr_new(arr);
	stack_push(ev);
	free(index);
}

static void
traverse_access(ast_node_t *tree)
{
	eval_t *evnum, *resev;
	id_item_t *item;
	ast_node_access_t *acc;
	int i, num;
	int *ind;
	char *name;

	ind = NULL;
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

	if (item->type != ID_ARR) {
		print_warn("%s not array\n", item->name);
		goto error;
	}

	if (item->arr->dims != acc->dims) {
		print_warn("array have %d dimentions\n", item->arr->dims);
		goto error;
	}

	ind = xmalloc(acc->dims * sizeof(*ind));

	for (i = 0; i < acc->dims; i++) {
		traverse(acc->ind[i]);

		if (nerrors != 0)
			goto error;
		
		evnum = stack_pop();
		if (evnum == NULL) {
			print_warn("cant get operand\n");
			nerrors++;
			goto error;
		}
		
		switch (evnum->type) {
		case EVAL_NUM:
			ind[i] = evnum->value;
			break;
		default:
			print_warn_and_die("INTERNAL_ERROR: cant traverse access\n");
		}
		
		eval_free(evnum);
	}


	if (arr_get_item(item->arr, ind, &num) != ret_ok) {
		print_warn("out of range\n");
		nerrors++;
		goto error;
	}

	resev = eval_num_new(num);
	stack_push(resev);
	
	ufree(ind);
	return;
error:
	ufree(ind);
	nerrors++;
}

static void
traverse_func_def(ast_node_t *tree)
{
	func_t *func;
	ast_node_func_t *synfunc;
	int ret;

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
			print_warn_and_die("cant delete from func table");
	}

	func = func_new(synfunc->name);

	func_set_args(func, synfunc->args, synfunc->nargs);
	func_set_body(func, synfunc->body);

	function_table_insert(func); 
}

void
exec_function(func_t *func)
{
	//FIXME: write me!
	ast_node_t *next;
	res_type_t res;

	next = func->body;

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
			return;
		default:
			next = next->child;
			continue;
		}
	}
	
}

//get next func argue
//if fails return errors and/or inc nerrors
id_item_t *
get_next_argue(ast_node_t *argnode, char *hint)
{
	eval_t *ev;
	id_item_t *item;

	/* warning! conflict when free */
	//try to get args by value(not by copy)
	if (argnode->type == AST_NODE_ID) {
		ast_node_id_t *id;
		id_item_t *res;
		id = (ast_node_id_t *) argnode;
		res = id_table_lookup_all(id->name);
		if (res == NULL) {
			print_warn("cant get id %s\n", id->name);
			nerrors++;
		}

		return res;
	}

	traverse(argnode);

	if (nerrors != 0)
		return NULL;
	
	ev = stack_pop();

	if (ev == NULL) {
		print_warn("cant get value\n");
		return NULL;
	}
	
	if (hint == NULL)
		print_warn_and_die("hint is NULL\n");
	
	item = id_item_new(hint);
	
	set_value_id(item, ev);
	
	eval_free(ev);

	return item;
}

/*
 * add args to func array, then
 * try to execute library function
 * WARN: we must be care with global args(passed by name)
 * bcs may be double free situation
 */
void
exec_library_function(func_t *func, ast_node_func_call_t *call)
{
	id_item_t **items;
	int err, i, rtype;
	void *rval;
	
	items = NULL;

	if (call->nargs != 0) {
		items = xmalloc(sizeof(*items) * func->nargs);
		memset(items, 0, sizeof(*items) * func->nargs);
	}

	for (i = 0; i < func->nargs; i++) {
		//FIXME: now we use empty name to indicate
		//that variable didn't inserted to scope
		//may be more usefull to pass at libcall
		//new type?
		items[i] = get_next_argue(call->args[i], "");

		//FIXME: may be memory leak
		if (items[i] == NULL || nerrors != 0)
			goto finalize;
		}

	//double free(if we exec libcall_del)
	err = func->handler(items, &rtype, &rval);
	if (err != 0) {
		print_warn("error when exec %s\n", func->name);
		nerrors++;
	}
	
finalize:

	//destruct args
	//NOTE: we can't touch not internal args
	for (i = 0; i < func->nargs; i++) {
		if (items[i] != NULL && 
		    strcmp(items[i]->name, "") == 0)
			id_item_free(items[i]);
	}
	
	ufree(items);
	
	return;
}

/* NOTE:
 * Don't remove thrid argue!
 * When we use recursion we must get args from
 * previous scope and push in new one, then we need to
 * call id_table_push(new scope).
 */
static void
add_args_to_scope(func_t *func, ast_node_func_call_t *call,
    struct hash_table *scope)
{
	assert(func != NULL && func->args != NULL);

	int i;
	id_item_t *item;
	char *name;

	for (i = 0; i < func->nargs; i++) {
		name = func->args[i];
		if (name == NULL)
			print_warn_and_die("NULL name ptr\n");

		item = get_next_argue(call->args[i], name);

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

	if (call->nargs != func->nargs) {
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
	} else
		res = RES_OK;
	
	if (res == RES_ERROR)
		nerrors++;

	return res;
}

void
traverse_scope(ast_node_t *tree)
{
	ast_node_t *next;
	res_type_t res;
	struct hash_table *idtable;

	idtable = id_table_create();
	id_table_push(idtable);
	
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
			goto finalize;
		default:
			next = next->child;
			continue;
		}
	}

finalize:
	id_table_pop();
	id_table_free(idtable);

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
set_value_id(id_item_t *item, eval_t *ev)
{
		
	switch (ev->type) {
	case EVAL_NUM:
		id_item_set(item, ID_NUM, &(ev->value));
		break;
	case EVAL_ARR:
		id_item_set(item, ID_ARR, ev->arr);
		break;
	default:
		print_warn_and_die("WIP\n");
	}
}


static void
set_value_node_access(ast_node_access_t *node, eval_t *newval)
{
	assert(newval != NULL);

	eval_t *ev;
	id_item_t *item;
	int *ind;
	int i;

	ev = NULL;
	ind = NULL;

	if (newval->type != EVAL_NUM) {
		print_warn("try assign to arr not number\n");
		goto err;
	}
	
	item = id_table_lookup_all(node->name);
	if (item == NULL) {
		print_warn("symb undefined\n");
		goto err;
	}

	if (item->type != ID_ARR) {
		print_warn("%s is not array\n", item->name);
		goto err;
	}
	
	ind = xmalloc(node->dims * sizeof(*ind));

	for (i = 0; i < node->dims; i++) {
		traverse(node->ind[i]);

		if (nerrors != 0)
			goto err;

		ev = stack_pop();
		if (ev == NULL) {
			print_warn("can't get index\n");
			goto err;
		}
		if (ev->type != EVAL_NUM) {
			print_warn("index must be number\n");
			goto err;
		}
		ind[i] = ev->value;

		eval_free(ev);
	}

	if (arr_set_item(item->arr, ind, newval->value) != ret_ok) {
		print_warn("out of range\n");
		goto err;
	}

	ufree(ind);

	return;
err:
	nerrors++;

	eval_free(ev);
	ufree(ind);

}

static void
set_value_node(ast_node_t *ltree, eval_t *ev)
{
	ast_node_id_t *id;
	ast_node_access_t *acc;

	switch (ltree->type) {
	case AST_NODE_ID: {
		id_item_t *item;
		
		id = (ast_node_id_t *)ltree;

		item = id_table_lookup_all(id->name);
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
traverse_as_rest(ast_node_t *tree)
{
	ast_node_t *ltree;
	eval_t *right;

	ltree = (ast_node_t *)tree->left;

	if (tree->right->type == AST_NODE_AS)
		traverse_as_rest(tree->right);
	else 
		traverse(tree->right);

	if (nerrors != 0)
		return;

	right = stack_pop();
	if (right == NULL) {
		nerrors++;
		print_warn("cant get operand\n");
		return;
	}

	set_value_node(ltree, right);

	if (nerrors != 0)
		return;

	stack_push(right);
}

//FIXME: now just simple fix problem
//with multiple assignment
// in top assignment we must pop last eval
static void
traverse_as(ast_node_t *tree)
{
	return_if_fail(tree != NULL);

	eval_t *right;

	traverse_as_rest(tree);

	if (nerrors != 0)
		return;

	right = stack_pop();
	if (right == NULL) {
		nerrors++;
		print_warn("cant get operand");
		return;
	}

	eval_free(right);
}

static void
traverse_unary(ast_node_t *tree)
{
	ast_node_unary_t *unary;
	eval_t *ev, *res;

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

	res = NULL;

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
	ast_node_return_t *rettree;

	rettree = (ast_node_return_t *) tree;

	if (rettree->retval == NULL) {
		//NOTE: if func res is NULL and it
		//assign to some val then runtime err
		return;
	} else {
		traverse(rettree->retval);
		//FIXME: return check?
		return;
	}
	
	helper.is_return++;
	print_warn_and_die("return node traverse WIP!\n");
}

static void
traverse_import(ast_node_t *tree)
{
	print_warn_and_die("WIP\n");
}

static void
traverse_break(ast_node_t *tree)
{
	helper.is_break++;
}

static void
traverse_continue(ast_node_t *tree)
{
	helper.is_continue++;
}

static void
traverse_stub(ast_node_t *tree)
{
	return;
}

struct {
	ast_type_t node;
	traverse_cb callback;
} node_type [] = {
	{AST_NODE_NUM, traverse_num},
	{AST_NODE_ID, traverse_id},
	{AST_NODE_ARR, traverse_arr},
	{AST_NODE_ACCESS, traverse_access},
	{AST_NODE_DEF, traverse_func_def},
	{AST_NODE_CALL, traverse_func_call},
	{AST_NODE_UNARY, traverse_unary},
	{AST_NODE_OP, traverse_op},
	{AST_NODE_OP_AS, traverse_op_as},
	{AST_NODE_AS, traverse_as},
	{AST_NODE_IF, traverse_if},
	{AST_NODE_FOR, traverse_for},
	{AST_NODE_WHILE, traverse_while},
	{AST_NODE_DO, traverse_do},
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
	return_if_fail(tree != NULL);

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
		stack_flush((stack_item_free_t )eval_free);
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

