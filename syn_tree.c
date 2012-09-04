#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "macros.h"
#include "syn_tree.h"


static void
ast_node_free(ast_node_t *tree)
{
	ast_node_access_t *acc;
	ast_node_arr_t *arr;
	ast_node_id_t *id;
	ast_node_if_t *ifnode;
	ast_node_for_t *fornode;
	ast_node_while_t *whilenode;
	ast_node_do_t *donode;
	ast_node_unary_t *unary;
	ast_node_return_t *ret;

	int i, n;

	return_if_fail(tree != NULL);
	
	switch (tree->type) {
	case AST_NODE_NUM:
		break;
	case AST_NODE_ID:
		id = (ast_node_id_t *)tree;
		ufree(id->name);
		break;
	case AST_NODE_ARR:
		//FIXME: CHECK_ME
		arr = (ast_node_arr_t *)tree;
		n = arr->sz;
		for (i = 0; i < n; i++)
			ast_node_unref(arr->arr[i]);
		ufree(arr->len);
		ufree(arr->arr);
		break;
	case AST_NODE_ACCESS:
		acc = (ast_node_access_t *)tree;
		ufree(acc->name);
		for (i = 0; i < acc->dims; i++)
			ast_node_unref(acc->ind[i]);
		ufree(acc->ind);
		break;
	case AST_NODE_RETURN:
		ret = (ast_node_return_t *)tree;
		ast_node_unref(ret->retval);
		break;
	case AST_NODE_IF:
		ifnode = (ast_node_if_t *)tree;
		ast_node_unref(ifnode->_if);
		ast_node_unref(ifnode->body);
		ast_node_unref(ifnode->_else);
		break;
	case AST_NODE_WHILE:
		whilenode = (ast_node_while_t *)tree;
		ast_node_unref(whilenode->cond);
		ast_node_unref(whilenode->body);
		break;
	case AST_NODE_DO:
		donode = (ast_node_do_t *)tree;
		ast_node_unref(donode->cond);
		ast_node_unref(donode->body);
		break;

	case AST_NODE_FOR:
		fornode = (ast_node_for_t *)tree;
		ast_node_unref(fornode->expr1);
		ast_node_unref(fornode->expr2);
		ast_node_unref(fornode->expr3);
		ast_node_unref(fornode->body);
		break;
	case AST_NODE_UNARY:
		unary = (ast_node_unary_t *) tree;
		ast_node_unref(unary->node);
		break;
	case AST_NODE_OP:
	case AST_NODE_OP_AS:
	case AST_NODE_AS:
	case AST_NODE_SCOPE:
	case AST_NODE_STUB:	
	case AST_NODE_BREAK:
	case AST_NODE_CONTINUE:
		ast_node_unref(tree->left);
		ast_node_unref(tree->right);
		break;
	default:
		print_warn_and_die("something wrong, no such type\n");
	}

	ast_node_unref(tree->child);

	ufree(tree);
}

static void
ast_node_func_free(ast_node_t *tree)
{
	ast_node_func_call_t *call;
	ast_node_func_t *func;
	int i;

	return_if_fail(tree != NULL);


	switch (tree->type) {
	case AST_NODE_DEF:

		func = (ast_node_func_t *)tree;
		ufree(func->name);

		for (i = 0; i < func->nargs; i++)
			ufree(func->args[i]);
		ufree(func->args);

		//now you must free body manualy!
		//ast_node_unref(AST_NODE(func->body));

		break;
	case AST_NODE_CALL:
		call = (ast_node_func_call_t *) tree;
		ufree(call->name);
		for (i = 0; i < call->nargs; i++)
			ast_node_unref(call->args[i]);
		ufree(call->args);

		break;
	default:
		print_warn_and_die("something wrong, no such type\n");
	}
	ast_node_unref(tree->child);

	ufree(tree);
}

ast_node_t *
ast_node_new(ast_type_t type, int sz,
    destructor_t destructor)
{
	ast_node_t *res;

	res = malloc_or_die(sz);
	memset(res, 0, sz);

	res->type = type;
	res->destructor = destructor;

	return res;
}


/*
ast_node_t *
ast_node_copy(ast_node_t *node)
{
	ast_node_t **ind;
	ast_node_t *left, *right;
	char *name;
	int i;

	switch (node->type) {
	case AST_NODE_OP: {
		ast_node_op_t *op;

		op = (ast_node_op_t *) node;

		left = ast_node_copy(node->left);
		right = ast_node_copy(node->right);

		return ast_node_op_new(left, right, op->opcode);
	}
	case AST_NODE_AS: {
		left = ast_node_copy(node->left);
		right = ast_node_copy(node->right);

		return ast_node_as_new(left, right);
	}
	case AST_NODE_ID: {
		ast_node_id_t *id;

		id = (ast_node_id_t *) node;
		name = strdup_or_die(id->name);
		return ast_node_id_new(name);
	}
	case AST_NODE_NUM: {
		ast_node_num_t *num;

		num = (ast_node_num_t *) node;

		return ast_node_num_new(num->num);
	}
	case AST_NODE_ACCESS: {
		ast_node_access_t *acc;
		
		acc = (ast_node_access_t *) node;
		name = strdup_or_die(acc->name);
		
		ind = malloc_or_die(sizeof(*ind) * acc->dims);
		for (i = 0; i < acc->dims; i++)
			ind[i] = ast_node_copy(acc->ind[i]);
		
		//print_warn_and_die("WIP!\n");
		return ast_node_access_new(name, acc->dims, ind);
	}
	case AST_NODE_ARR: {
		ast_node_arr_t *arr;
		ast_node_t **elems;
		int *len;

		arr = (ast_node_arr_t *) node;

		elems = malloc_or_die(arr->sz * sizeof(*elems));

		for (i = 0; i< arr->sz; i++)
			elems[i] = ast_node_copy(arr->arr[i]);
		
		len = malloc_or_die(arr->dims * sizeof(*len));
		memcpy(len, arr->len, arr->dims * sizeof(*len));
	
		return ast_node_arr_new(elems, arr->dims, len, arr->sz);	   
	}
	case AST_NODE_SCOPE: {
	
	}
	case AST_NODE_IF: {
	
	}
	case AST_NODE_FOR: {
	
	}
	case AST_NODE_WHILE: {
	
	}
	
	case AST_NODE_DEF: {
	
		print_warn_and_die("WIP! write me\n");
	}
	
	case AST_NODE_CALL: {
		ast_node_func_call_t *call;
		ast_node_func_call_t *ncall;
		
		call = (ast_node_func_call_t *) node;

		name = strdup_or_die(call->name);

		ncall = (ast_node_func_call_t *) ast_node_func_call_new(name);

		for (i = 0; i < call->nargs; i++) {
			ast_node_t *arg;

			arg = ast_node_copy(call->args[i]);

			ast_node_func_call_add_next_arg(ncall, arg);
		}

		return AST_NODE(ncall);
	}
	case AST_NODE_BREAK:
	case AST_NODE_CONTINUE:
	case AST_NODE_RETURN:
	case AST_NODE_STUB: {
		ast_node_t *res;

		res = malloc_or_die(sizeof(*node));
		memcpy(res, node, sizeof(*res));

		return res;
	}
	default:
		print_warn_and_die("Internal error!\n");
	}
	return ast_node_stub_new();
}
*/

ast_node_t *
ast_node_num_new(int num)
{
	ast_node_num_t *res;
	
	res = (ast_node_num_t *) 
	    ast_node_new(AST_NODE_NUM, sizeof(*res), ast_node_free);

	res->num = num;
	
	return AST_NODE(res);
}

ast_node_t *
ast_node_id_new(char *name)
{
	ast_node_id_t *res;
	
	res = (ast_node_id_t *) 
	    ast_node_new(AST_NODE_ID, sizeof(*res), ast_node_free);

	res->name = name;

	return AST_NODE(res);
}

ast_node_t *
ast_node_arr_new(ast_node_t **arr, int dims, int *len, int sz)
{
	ast_node_arr_t *res;

	res = (ast_node_arr_t *) 
	    ast_node_new(AST_NODE_ARR, sizeof(*res), ast_node_free);

	res->arr = arr;
	res->dims = dims;
	res->sz = sz;

	res->len = len;

	return AST_NODE(res);
}

ast_node_t *
ast_node_func_def_new(char *name)
{
	ast_node_func_t *res;

	res = (ast_node_func_t *) 
	    ast_node_new(AST_NODE_DEF, sizeof(*res), ast_node_func_free);

	res->name = name;
	res->args = NULL;
	res->nargs = 0;

	return AST_NODE(res);
}

ast_node_t *
ast_node_func_call_new(char *name)
{
	ast_node_func_call_t *res;

	res = (ast_node_func_call_t *) 
	    ast_node_new(AST_NODE_CALL, sizeof(*res), ast_node_func_free);

	res->name = name;

	return AST_NODE(res);
}
	
void
ast_node_func_call_add_next_arg(ast_node_func_call_t *call, ast_node_t *node)
{
	int n;

	n = call->nargs++;

	call->args = realloc_or_die(call->args,
	    call->nargs * sizeof(*(call->args)));

	call->args[n] = node;
}

ast_node_t *
ast_node_if_new(ast_node_t *_if, ast_node_t *body, ast_node_t *_else)
{
	ast_node_if_t *res;

	res = (ast_node_if_t *)
	    ast_node_new(AST_NODE_IF, sizeof(*res), ast_node_free);

	res->_if = _if;
	res->body = body;
	res->_else = _else;

	return AST_NODE(res);
}

ast_node_t *
ast_node_for_new(ast_node_t *expr1, ast_node_t *expr2,
    ast_node_t *expr3, ast_node_t *body)
{
	ast_node_for_t *res;

	res = (ast_node_for_t *)
	    ast_node_new(AST_NODE_FOR, sizeof(*res), ast_node_free);
	
	res->expr1 = expr1;
	res->expr2 = expr2,
	res->expr3 = expr3;

	res->body = body;
	
	return AST_NODE(res);
}

ast_node_t *
ast_node_while_new(ast_node_t *cond, ast_node_t *body)
{
	ast_node_while_t *res;

	res = (ast_node_while_t *)
	    ast_node_new(AST_NODE_WHILE, sizeof(*res), ast_node_free);
	
	res->cond = cond;

	res->body = body;
	
	return AST_NODE(res);
}

ast_node_t *
ast_node_do_new(ast_node_t *cond, ast_node_t *body)
{
	ast_node_while_t *res;

	res = (ast_node_while_t *)
	    ast_node_new(AST_NODE_DO, sizeof(*res), ast_node_free);
	
	res->cond = cond;

	res->body = body;
	
	return AST_NODE(res);
}

ast_node_t *
ast_node_scope_new(ast_node_t *child)
{
	ast_node_scope_t *res;

	res = (ast_node_scope_t *)
	    ast_node_new(AST_NODE_SCOPE, sizeof(*res), ast_node_free);

	    AST_NODE(res)->child = child;

	    return AST_NODE(res);
}

ast_node_t *
ast_node_access_new(char *name, int dims, ast_node_t **ind)
{
	ast_node_access_t *res;

	res = (ast_node_access_t *) 
	    ast_node_new(AST_NODE_ACCESS, sizeof(*res), ast_node_free);

	res->name = name;
	res->dims = dims;
	res->ind = ind;

	return AST_NODE(res);
}

ast_node_t *
ast_node_unary_new(ast_node_t *node, opcode_t opcode)
{
	ast_node_unary_t *res;

	res = (ast_node_unary_t *) 
	    ast_node_new(AST_NODE_UNARY, sizeof(*res), ast_node_free);

	res->opcode = opcode;
	res->node = node;

	return AST_NODE(res);
}

ast_node_t *
ast_node_op_new(ast_node_t *left, ast_node_t *right, opcode_t opcode)
{
	ast_node_op_t *res;
	
	res = (ast_node_op_t *) 
	    ast_node_new(AST_NODE_OP, sizeof(*res), ast_node_free);
	
	AST_NODE(res)->left = left;
	AST_NODE(res)->right = right;

	res->opcode = opcode;

	return AST_NODE(res);
}

ast_node_t *
ast_node_op_as_new(ast_node_t* left, ast_node_t *right, opcode_t opcode)
{
	ast_node_op_t *res;
	
	res = (ast_node_op_t *) 
	    ast_node_new(AST_NODE_OP_AS, sizeof(*res), ast_node_free);
	
	AST_NODE(res)->left = left;
	AST_NODE(res)->right = right;

	res->opcode = opcode;

	return AST_NODE(res);
}

ast_node_t *
ast_node_as_new(ast_node_t *left, ast_node_t *right)
{
	ast_node_as_t *res;
	
	res = (ast_node_as_t *) 
	    ast_node_new(AST_NODE_AS, sizeof(*res), ast_node_free);

	AST_NODE(res)->left = left;
	AST_NODE(res)->right = right;

	return AST_NODE(res);
}

ast_node_t *
ast_node_return_new(ast_node_t *retval)
{
	ast_node_return_t *res;

	res = (ast_node_return_t *) 
	    ast_node_new(AST_NODE_RETURN, sizeof(*res), ast_node_free);

	res->retval = retval;
	
	return AST_NODE(res);
}

ast_node_t *
ast_node_break_new()
{
	ast_node_break_t *res;

	res = (ast_node_break_t *)
	    ast_node_new(AST_NODE_BREAK, sizeof(*res), ast_node_free);

	return AST_NODE(res);
}

ast_node_t *
ast_node_continue_new()
{
	ast_node_continue_t *res;

	res = (ast_node_continue_t *)
	    ast_node_new(AST_NODE_CONTINUE, sizeof(*res), ast_node_free);

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

