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
	//experimental
	int is_call;
	int is_cycle;
	//
	
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

static void
traverse_arr(ast_node_t *tree)
{
	arr_t *arr;
	eval_t *ev;
	ast_node_t **synarr;
	int i, sz, res;
	
	synarr = ((ast_node_arr_t *)tree)->arr;
	sz = ((ast_node_arr_t *)tree)->sz;

	//size of int
	arr = arr_new(sz, sizeof(int));

	for (i = 0; i < sz; i++) {
		traverse(synarr[i]);
		//error handle?
		if ((ev = stack_pop()) == NULL)
			print_warn_and_die("unexpected error\n");
		
		if (ev->type != EVAL_NUM)
			print_warn_and_die("cant set item\n");
		
		res = ev->value;

		if (arr_set_item(arr, i, res) != ret_ok)
			print_warn_and_die("cant set item\n");
	}
	
	ev = eval_arr_new(arr);
	stack_push(ev);
}

static void
traverse_access(ast_node_t *tree)
{
	eval_t *evnum, *resev;
	id_item_t *item;
	ast_node_access_t *acc;
	int ind, num;
	char *name;

	if (nerrors != 0)
		return;

	acc = (ast_node_access_t *)tree;

	name = acc->name;

	item = id_table_lookup_all(name);
	//FIXME
	if (item == NULL) {
		print_warn("symb %s undefined\n", name);
		nerrors++;
		return;
	}

	if (item->type != ID_ARR) {
		print_warn("%s not array\n", item->name);
		nerrors++;
		return;
	}

	traverse(acc->ind);
	
	evnum = stack_pop();
	
	switch (evnum->type) {
	case EVAL_NUM:
		ind = evnum->value;
		break;
	default:
		print_warn_and_die("INTERNAL_ERROR: cant traverse access\n");
	}

	if (arr_get_item(item->arr, ind, &num) != ret_ok) {
		print_warn("out of range\n");
		nerrors++;
		return;
	}

	resev = eval_num_new(num);
	stack_push(resev);
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
	//try to get argues by value(not by copy)
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

	//FIXME: rewrite me
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
 * add argues to func array, then
 * try to execute library function
 * WARN: we must be care with global argues(passed by name)
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
		items = malloc_or_die(sizeof(*items) * func->nargs);
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

	//destruct argues
	//NOTE: we can't touch not internal argues
	for (i = 0; i < func->nargs; i++) {
		if (items[i] != NULL && 
		    strcmp(items[i]->name, "") == 0)
			id_item_free(items[i]);
	}
	
	if (items != NULL)
		ufree(items);
	
	return;
}

/* NOTE:
 * dont remove thrid argue
 * when we use recursion we must get argues from
 * previous scope and push in new one, then we need to
 * exec id_table_push(new scope)
 */
static void
add_argues_to_scope(func_t *func, ast_node_func_call_t *call,
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
		add_argues_to_scope(func, call, idtable);

	if (nerrors != 0)
		goto finalize;

	id_table_push(idtable);
	
	exec_function(func);
	
	id_table_pop();

finalize:
	id_table_free(idtable);
}

res_type_t
traverse_body(ast_node_t *tree)
{

	traverse(tree);

	if (nerrors != 0)
		return RES_ERROR;
	else if (helper.is_continue > 0)
		return RES_CONTINUE;
	else if (helper.is_break > 0)
		return RES_BREAK;
	else if (helper.is_continue > 0)
		return RES_RETURN;
	else
		return RES_OK;
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
			print_warn("traverse scope err\n");
			goto finalize;
		case RES_CONTINUE:
			print_warn("unexpected continue\n");
			goto finalize;
		case RES_BREAK:
			print_warn("unexpected break\n");
			goto finalize;
		case RES_RETURN:
			print_warn("unexpected return\n");
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

void
traverse_cond(ast_node_t *tree)
{
	ast_node_if_t *ifnode;
	eval_t *ev;

	ifnode = (ast_node_if_t *) tree;
	
	traverse(ifnode->_if);
	if (nerrors != 0)
		return;

	ev = stack_pop();

	if (eval_is_zero(ev) == FALSE) {
		traverse(ifnode->body);
	} else if (ifnode->_else != NULL) {
		traverse(ifnode->_else);
	}

	eval_free(ev);
}

void
traverse_for(ast_node_t *tree)
{
	ast_node_for_t *fornode;
	eval_t *ev;

	fornode = (ast_node_for_t *)tree;

	traverse(fornode->expr1);

	if (nerrors != 0)
		return;

	while (1) {

		traverse(fornode->expr2);

		ev = stack_pop();

		if (eval_is_zero(ev) == TRUE)
			break;

		eval_free(ev);

		traverse(fornode->body);

		traverse(fornode->expr3);

		if (nerrors != 0)
			return;
	}

	eval_free(ev);
}

void
traverse_while(ast_node_t *tree)
{
	ast_node_while_t *whilenode;
	eval_t *ev;

	whilenode = (ast_node_while_t *) tree;

	while (1) {
		traverse(whilenode->cond);

		ev = stack_pop();

		if (eval_is_zero(ev) == TRUE)
			break;

		eval_free(ev);

		traverse(whilenode->body);

		if (nerrors != 0)
			return;
	}

	eval_free(ev);
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
set_value_node_access(ast_node_access_t *node, eval_t *ev)
{
	eval_t *ind;
	id_item_t *item;

	if (ev->type != EVAL_NUM) {
		print_warn("try assign to arr not number\n");
		nerrors++;
		return;
	}
	
	item = id_table_lookup_all(node->name);
	if (item == NULL) {
		print_warn("symb undefined\n");
		nerrors++;
		return;
	}

	if (item->type != ID_ARR) {
		print_warn("%s is not array\n", item->name);
		nerrors++;
		return;
	}
	
	traverse(node->ind);
	ind = stack_pop();
	
	if (ind->type != EVAL_NUM) {
		print_warn("index must be number\n");
		nerrors++;
		return;
	}

	if (arr_set_item(item->arr, ind->value, ev->value) != ret_ok) {
		print_warn("out of range\n");
		nerrors++;
		return;
	}
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
	eval_t *right;

	traverse_as_rest(tree);

	if (nerrors != 0)
		return;

	right = stack_pop();
	eval_free(right);
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
	
	res = eval_process_op(left, right, optree->opcode);

	eval_free(left);
	eval_free(right);
	
	if (res == NULL) {
		nerrors++;
		print_warn("operation error\n");
		return;
	}
	
	stack_push(res);
}

static void
traverse_return(ast_node_t *tree)
{
	helper.is_return++;
	print_warn_and_die("return node traverse WIP!\n");
}

static void
traverse_break(ast_node_t *tree)
{
	print_warn_and_die("return node traverse WIP!\n");
}

static void
traverse_continue(ast_node_t *tree)
{
	print_warn_and_die("return node traverse WIP!\n");
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
	{AST_NODE_AS, traverse_as},
	{AST_NODE_OP, traverse_op},
	{AST_NODE_ARR, traverse_arr},
	{AST_NODE_ACCESS, traverse_access},
	{AST_NODE_DEF, traverse_func_def},
	{AST_NODE_CALL, traverse_func_call},
	{AST_NODE_SCOPE, traverse_scope},
	{AST_NODE_IF, traverse_cond},
	{AST_NODE_FOR, traverse_for},
	{AST_NODE_WHILE, traverse_while},
	{AST_NODE_ID, traverse_id},
	{AST_NODE_NUM, traverse_num},
	{AST_NODE_RETURN, traverse_return},
	{AST_NODE_BREAK, traverse_break},
	{AST_NODE_CONTINUE, traverse_continue},
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

