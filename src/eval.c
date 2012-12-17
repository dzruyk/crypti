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

eval_t *
eval_var_new(int value)
{
	eval_t *res;
	
	res = xmalloc(sizeof(*res));
	res->type = EVAL_VAR;
	res->var = value;

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

void
eval_free(eval_t *eval)
{
	if (eval == NULL)
		return;
	
	switch(eval->type) {
	case EVAL_VAR:
		free(eval);
		break;
	//warning, may be memory leak
	case EVAL_ARR:
		free(eval);
		break;
	default:
		print_warn_and_die("WIP\n");
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
	int val;

	if (ev->type != EVAL_VAR)
		return NULL;

	val = ev->var;

	switch (opcode) {
	case OP_PLUS:
		break;
	case OP_MINUS:
		val = -val;
		break;
	case OP_NOT:
		val = !val;
		break;
	case OP_NOTNOT:
		val = !!val;
		break;
	default:
		SHOULDNT_REACH();
	}

	res = eval_num_new(val);

	return res;
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
		varop_mul(res, a, b);
		break;
	case OP_DIV:
		if (b == 0) {
			print_warn("divide by zero\n");
			return NULL;
		}
		varop_div(res, a, b);
		break;
	case OP_PLUS:
		varop_add(res, a, b);
		break;
	case OP_MINUS:
		varop_sub(res, a, b);
		break;
	case OP_EQ:
		res = (a == b);
		break;
	case OP_NEQ:
		res = (a != b);
		break;
	case OP_GR:
		res = (a > b);
		break;
	case OP_LO:
		res = (a < b);
		break;
	case OP_GE:
		res = (a >= b);
		break;
	case OP_LE:
		res = (a <= b);
		break;
	case OP_L_AND:
		res = (a && b);
		break;
	case OP_L_OR:
		res = (a || b);
		break;
	case OP_B_OR:
		varop_or(res, a, b);
		break;
	case OP_B_XOR:
		varop_xor(res, a, b);
		break;
	case OP_B_AND:
		varop_and(res, a, b);
		break;
	case OP_SHL:
		varop_shl(res, a, b);
		break;
	case OP_SHR:
		varop_shr(res, a, b);
		break;
	default:
		SHOULDNT_REACH();
	}

	ev = eval_var_new(res);

	return ev;
}


ret_t
eval_print_val(eval_t *eval)
{
	int value;

	if (eval == NULL)
		return ret_err;	

	switch(eval->type) {
	case EVAL_VAR:
		value = eval->var;
		printf("%d\n", value);
		break;
	case EVAL_ARR:
		arr_print(eval->arr);	
		break;
	default:
		print_warn_and_die("INTERNAL ERROR: cant get value\n");
	}

	return ret_ok; 
}

