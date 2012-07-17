#include "array.h"
#include "common.h"
#include "eval.h"
#include "id_table.h"
#include "lex.h"
#include "macros.h"
#include "stack.h"

#include "traverse.h"

static void traverse(ast_node_t *tree);

typedef void (*traverse_cb)(ast_node_t *tree);

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

	item = id_table_lookup(name);
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

	item = id_table_lookup(name);
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

void
traverse_func_def(ast_node_t *tree)
{
	print_warn_and_die("WIP\n");
}

void
traverse_func_call(ast_node_t *tree)
{
	print_warn_and_die("WIP\n");
}

void
set_value_id(ast_node_id_t *node, eval_t *ev)
{
	id_item_t *item;

	item = id_table_lookup(node->name);
	//if we havent id, then we must define it
	if (item == NULL) {
		item = malloc_or_die(sizeof(*item));
		item->name = strdup_or_die(node->name);
		item->type = ID_UNKNOWN;

		id_table_insert(item);
	}
		
	switch (ev->type) {
	case EVAL_NUM:
		item->type = ID_NUM;
		item->value = ev->value;
		break;
	case EVAL_ARR:
		item->type = ID_ARR;
		item->arr = ev->arr;
		break;
	default:
		print_warn_and_die("WIP\n");
	}
}


static void
set_value_access(ast_node_access_t *node, eval_t *ev)
{
	eval_t *ind;
	id_item_t *item;

	if (ev->type != EVAL_NUM) {
		print_warn("try assign to arr not number\n");
		nerrors++;
		return;
	}
	
	item = id_table_lookup(node->name);
	if (item == NULL) {
		print_warn("symb undefined\n");
		nerrors++;
		return;
	}
	
	traverse(node->ind);
	ind = stack_pop();
	
	if (ind->type != EVAL_NUM) {
		print_warn("try assign to arr not number\n");
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
set_value(ast_node_t *ltree, eval_t *ev)
{
	ast_node_id_t *id;
	ast_node_access_t *acc;

	switch (ltree->type) {
	case AST_NODE_ID:
		
		id = (ast_node_id_t *)ltree;
		set_value_id(id, ev);

		break;
	case AST_NODE_ACCESS:
		
		acc = (ast_node_access_t *)ltree;
		set_value_access(acc, ev);
		
		break;
	default:
		print_warn("assignment to not variable\n");
		nerrors++;	
	}
}

static void
traverse_as(ast_node_t *tree)
{
	ast_node_t *ltree;
	eval_t *right;

	ltree = (ast_node_t *)tree->left;

	traverse(tree->right);
	
	if (nerrors != 0)
		return;
	
	right = stack_pop();
	
	set_value(ltree, right);

	stack_push(right);
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
		return;
	}
	
	stack_push(res);
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
	{AST_NODE_ID, traverse_id},
	{AST_NODE_NUM, traverse_num},
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
	
	nerrors = 0;

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

