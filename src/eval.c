#include <assert.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "eval.h"
#include "log.h"
#include "macros.h"
#include "variable.h"
#include "var_op.h"

eval_t *
eval_var_new(void *var)
{
	eval_t *res;
	
	res = xmalloc(sizeof(*res));
	res->type = EVAL_VAR;
	res->var = var;

	return res;
}

eval_t *
eval_arr_new(arr_t *arr)
{
	eval_t *res;
	
	res = xmalloc(sizeof(*res));
	res->type = EVAL_ARR;
	res->arr = arr;

	return res;
}

/*
eval_t *
eval_dup(eval_t *src)
{
	eval_t *dst;

	dst = xmalloc(sizeof(*dst));
	
	switch(src->type) {
	case EVAL_VAR:
		dst->type = EVAL_VAR;
		dst->var = xmalloc(sizeof(*dst->var));
		var_init(dst->var);
		var_copy(dst->var, src->var);
		break;
	case EVAL_ARR:
		dst->type = EVAL_ARR;
		error(1, "WIP, I do something in next commit, promise\n");
	default:
		SHOULDNT_REACH();
	}

	return dst;
}
*/

void
eval_free(eval_t *eval)
{
	if (eval == NULL)
		return;
	
	switch(eval->type) {
	case EVAL_VAR:
		var_clear(eval->var);
		ufree(eval->var);
		ufree(eval);
		break;
	case EVAL_ARR:
		arr_free(eval->arr);
		ufree(eval);
		break;
	default:
		SHOULDNT_REACH();
	}
}


boolean_t
eval_is_zero(eval_t *eval)
{
	struct variable *var;
	mpl_int *mp;

	assert(eval != NULL);

	var = eval->var;
	mp = var_bignum_ptr(var);

	//FIXME: WIP, may be some errors
	if (eval->type != EVAL_VAR)
		return FALSE;
	else if (mpl_iszero(mp))
		return TRUE;
	else
		return FALSE;
}

eval_t *
eval_process_unary(eval_t *ev, opcode_t opcode)
{
	assert(ev != NULL);

	eval_t *res;
	struct variable *resvar;
	struct variable *var;
	int ret;

	if (ev->type != EVAL_VAR)
		return NULL;

	var = ev->var;
	
	resvar = xmalloc(sizeof(*resvar));
	var_init(resvar);

	switch (opcode) {
	case OP_PLUS:
		var_copy(resvar, var);
		break;
	case OP_MINUS:
		ret = varop_neg(resvar, var);
		if (ret != 0)
			goto error;
		break;
	case OP_NOT:
		ret = varop_not(resvar, var);
		if (ret != 0)
			goto error;
		break;
	case OP_NOTNOT:
		ret = varop_not(resvar, var);
		if (ret != 0)
			goto error;
		ret = varop_not(resvar, var);
		if (ret != 0)
			goto error;
		break;
	default:
		SHOULDNT_REACH();
	}

	res = eval_var_new(resvar);

	return res;

error:
	error(1, "WIP");
}

// FIXME: without error checks
eval_t *
eval_process_op(eval_t *left, eval_t *right, opcode_t opcode)
{
	assert(left != NULL && right != NULL);

	struct variable *a, *b, *res;
	eval_t *ev;
	int ret;
	
	//FIXME
	if (left->type != EVAL_VAR)
		return NULL;
	if (right->type != EVAL_VAR)
		return NULL;
	
	res = xmalloc(sizeof(*res));
	var_init(res);

	a = left->var;
	b = right->var;

	switch(opcode) {
#define CASE_ITEM(OP, handler)				\
	case OP:					\
		ret = handler(res, a, b);		\
		if (ret != 0)				\
			goto error;			\
		break

	CASE_ITEM(OP_POW, varop_pow);
	CASE_ITEM(OP_MUL, varop_mul);
	CASE_ITEM(OP_DIV, varop_div);
	CASE_ITEM(OP_PLUS, varop_add);
	CASE_ITEM(OP_MINUS, varop_sub);
	CASE_ITEM(OP_B_OR, varop_or);
	CASE_ITEM(OP_B_XOR, varop_xor);
	CASE_ITEM(OP_B_AND, varop_and);
	CASE_ITEM(OP_SHL, varop_shl);
	CASE_ITEM(OP_SHR, varop_shr);
	CASE_ITEM(OP_STR_CONCAT, varop_str_concat);
	CASE_ITEM(OP_OCTSTR_CONCAT, varop_oct_concat);

	case OP_EQ:
		ret = varop_cmp(a, b);
		if (ret == 0)
			var_set_one(res);
		else
			var_set_zero(res);

		break;
	case OP_NEQ:
		ret = varop_cmp(a, b);
		if (ret == 0)
			var_set_zero(res);
		else
			var_set_one(res);
		break;
	case OP_GR:
		ret = varop_cmp(a, b);
		if (ret == 1)
			var_set_one(res);
		else
			var_set_zero(res);
		break;
	case OP_LO:
		ret = varop_cmp(a, b);
		if (ret == -1)
			var_set_one(res);
		else
			var_set_zero(res);
		break;
	case OP_GE:
		ret = varop_cmp(a, b);
		if (ret == 1 || ret == 0)
			var_set_one(res);
		else
			var_set_zero(res);
		break;
	case OP_LE:
		ret = varop_cmp(a, b);
		if (ret == -1 || ret == 0)
			var_set_one(res);
		else
			var_set_zero(res);
		break;
	case OP_L_AND:
		if (varop_is_true(a) && varop_is_true(b))
			var_set_one(res);
		else
			var_set_zero(res);
		break;
	case OP_L_OR:
		if (varop_is_true(a) || varop_is_true(b))
			var_set_one(res);
		else
			var_set_zero(res);
		break;
	default:
		SHOULDNT_REACH();
	}

	ev = eval_var_new(res);

	return ev;

error:
	error(1, "eval_op error!\n");
}


ret_t
eval_print_val(eval_t *eval)
{
	struct variable *var;
	str_t *str;

	if (eval == NULL)
		return ret_err;	

	switch(eval->type) {
	case EVAL_VAR:
		var = eval->var;
		str = var_cast_to_str(var);
		printf("\"%s\"\n", str_ptr(str));
		break;
	case EVAL_ARR:
		//FIXME: stupid stub
		arr_print(eval->arr);	
		break;
	default:
		error(1, "INTERNAL ERROR: cant get value\n");
	}

	return ret_ok; 
}

