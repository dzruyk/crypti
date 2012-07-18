#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "macros.h"
#include "syn_tree.h"

static void
ast_node_free(ast_node_t *tree)
{
	ast_node_arr_t *arr;
	ast_node_access_t *acc;
	ast_node_id_t *id;

	int i, n;

	return_if_fail(tree != NULL);
	
	switch (tree->type) {
	case AST_NODE_NUM:
		break;
	case AST_NODE_ID:
		id = (ast_node_id_t *)tree;
		free(id->name);
		break;
	case AST_NODE_ARR:
		//CHECK_ME
		arr = (ast_node_arr_t *)tree;
		n = arr->sz;
		for (i = 0; i < n; i++)
			ast_node_unref(arr->arr[i]);
		break;
	case AST_NODE_ACCESS:
		acc = (ast_node_access_t *)tree;
		free(acc->name);
		ast_node_unref(acc->ind);
		break;
	case AST_NODE_OP:
	case AST_NODE_AS:
	case AST_NODE_STUB:	
		ast_node_unref(tree->left);
		ast_node_unref(tree->right);
		break;
	default:
		print_warn_and_die("something wrong, no such type\n");
	}

	free(tree);
}

static void
ast_node_func_free(ast_node_t *tree)
{
	ast_node_func_call_t *call;
	ast_node_func_t *func;
	return_if_fail(tree != NULL);


	switch (tree->type) {
	case AST_NODE_DEF:
		func = (ast_node_func_t *)tree;
		ufree(func->name);
		if (func->args == NULL)
			list_destroy(&(func->args), ufree);

		//now you must free body manualy!
		//ast_node_unref(AST_NODE(func->body));

		break;
	case AST_NODE_CALL:
		call = (ast_node_func_call_t *) tree;
		ufree(call->name);
		list_destroy(&(func->args), 
		    (data_destroy_func_t )ast_node_unref);

		break;
	default:
		print_warn_and_die("something wrong, no such type\n");
	}

	ufree(tree);
}

ast_node_t *
ast_node_num_new(int num)
{
	ast_node_num_t *res;
	
	res = malloc_or_die(sizeof(*res));
	memset(res, 0, sizeof(*res));

	AST_NODE(res)->type = AST_NODE_NUM;
	AST_NODE(res)->destructor = ast_node_free;
	
	res->num = num;
	
	return AST_NODE(res);
}

ast_node_t *
ast_node_id_new(char *name)
{
	ast_node_id_t *res;
	
	res = malloc_or_die(sizeof(*res));
	memset(res, 0, sizeof(*res));

	AST_NODE(res)->type = AST_NODE_ID;
	AST_NODE(res)->destructor = ast_node_free;

	res->name = name;

	return AST_NODE(res);
}


ast_node_t *
ast_node_arr_new(ast_node_t **arr, int sz)
{
	ast_node_arr_t *res;

	res = malloc_or_die(sizeof(*res));
	
	AST_NODE(res)->type = AST_NODE_ARR;
	AST_NODE(res)->destructor = ast_node_free;

	res->arr = arr;
	res->sz = sz;

	return AST_NODE(res);
}

ast_node_t *
ast_node_func_def(char *name)
{
	ast_node_func_t *res;

	res = malloc_or_die(sizeof(*res));
	
	memset(res, 0, sizeof(*res));

	AST_NODE(res)->type = AST_NODE_DEF;
	AST_NODE(res)->destructor = ast_node_func_free;

	res->name = name;
	res->args = list_init();
	if (res->args == NULL)
		print_warn_and_die("some_error\n");

	return AST_NODE(res);
}

ast_node_t *
ast_node_func_call(char *name)
{
	ast_node_func_call_t *res;

	res = malloc_or_die(sizeof(*res));
	
	AST_NODE(res)->type = AST_NODE_DEF;
	AST_NODE(res)->destructor = ast_node_func_free;

	res->name = name;

	res->args = list_init();
	if (res->args == NULL)
		print_warn_and_die("some_error\n");

	return AST_NODE(res);
}

ast_node_t *
ast_node_access_new(char *name, ast_node_t *ind)
{
	ast_node_access_t *res;

	res = malloc_or_die(sizeof(*res));
	
	AST_NODE(res)->type = AST_NODE_ACCESS;
	AST_NODE(res)->destructor = ast_node_free;

	res->name = name;
	res->ind = ind;

	return AST_NODE(res);
}


ast_node_t *
ast_node_op_new(ast_node_t *left, ast_node_t *right, opcode_t opcode)
{
	ast_node_op_t *res;
	
	res = malloc_or_die(sizeof(*res));
	
	AST_NODE(res)->type = AST_NODE_OP; 
	AST_NODE(res)->left = left;
	AST_NODE(res)->right = right;
	AST_NODE(res)->destructor = ast_node_free;

	res->opcode = opcode;

	return AST_NODE(res);
}

ast_node_t *
ast_node_as_new(ast_node_t *left, ast_node_t *right)
{
	ast_node_as_t *res;
	
	res = malloc_or_die(sizeof(*res));
	
	AST_NODE(res)->type = AST_NODE_AS; 
	AST_NODE(res)->left = left;
	AST_NODE(res)->right = right;
	AST_NODE(res)->destructor = ast_node_free;

	return AST_NODE(res);
}

ast_node_t *
ast_node_stub_new()
{
	ast_node_stub_t *res;
	
	res = malloc_or_die(sizeof(*res));

	memset(res, 0, sizeof(*res));
	AST_NODE(res)->type = AST_NODE_STUB;
	AST_NODE(res)->destructor = ast_node_free;

	return AST_NODE(res);
}

void
ast_node_unref(ast_node_t *tree)
{
	if (tree != NULL)
		tree->destructor(tree);
}
