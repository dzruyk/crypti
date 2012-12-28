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
		print_warn_and_die("WIP, I do something in next commit, promise\n");
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
	//TODO:warning, may be memory leak
	case EVAL_ARR:
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
	mp_int *mp;

	assert(eval != NULL);

	var = eval->var;
	mp = var_bignum_ptr(var);

	//FIXME: WIP, may be some errors
	if (eval->type != EVAL_VAR)
		return FALSE;
	else if (mp_iszero(mp))
		return TRUE;
	else
		return FALSE;
}

eval_t *
eval_process_unary(eval_t *ev, opcode_t opcode)
{
	assert(ev != NULL);

	eval_t *res;
	struct variable *var;
	int ret;

	if (ev->type != EVAL_VAR)
		return NULL;

	var = ev->var;

	switch (opcode) {
	case OP_PLUS:
		break;
	case OP_MINUS:
		ret = varop_neg(var, var);
		if (ret != 0)
			goto error;
		break;
	case OP_NOT:
		ret = varop_not(var, var);
		if (ret != 0)
			goto error;
		break;
	case OP_NOTNOT:
		ret = varop_not(var, var);
		if (ret != 0)
			goto error;
		ret = varop_not(var, var);
		if (ret != 0)
			goto error;
		break;
	default:
		SHOULDNT_REACH();
	}

	res = eval_var_new(var);

	return res;

error:
	print_warn_and_die("WIP");
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
	case OP_MUL:
		ret = varop_mul(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_DIV:
		if (b == 0) {
			print_warn("divide by zero\n");
			return NULL;
		}
		ret = varop_div(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_PLUS:
		ret = varop_add(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_MINUS:
		ret = varop_sub(res, a, b);
		if (ret != 0)
			goto error;
		break;
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
	case OP_B_OR:
		ret = varop_or(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_B_XOR:
		ret = varop_xor(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_B_AND:
		ret = varop_and(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_SHL:
		ret = varop_shl(res, a, b);
		if (ret != 0)
			goto error;
		break;
	case OP_SHR:
		ret = varop_shr(res, a, b);
		if (ret != 0)
			goto error;
		break;
	default:
		SHOULDNT_REACH();
	}

	ev = eval_var_new(res);

	return ev;

error:
	print_warn_and_die("WIP!\n");
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
		printf("%s\n", str_ptr(str));
		break;
	case EVAL_ARR:
		//FIXME: stupid stub
		arr_print(eval->arr, NULL);	
		break;
	default:
		print_warn_and_die("INTERNAL ERROR: cant get value\n");
	}

	return ret_ok; 
}

