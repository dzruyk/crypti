#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "crypti.h"
#include "eval.h"
#include "macros.h"


eval_t ev_zero = {
	EVAL_NUM,
	{0},
};

eval_t *
eval_num_new(int value)
{
	eval_t *res;
	
	res = malloc_or_die(sizeof(*res));
	res->type = EVAL_NUM;
	res->value = value;

	return res;
}

eval_t *
eval_arr_new(arr_t *arr)
{
	eval_t *res;
	
	res = malloc_or_die(sizeof(*res));
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
	case EVAL_NUM:
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
	assert(eval != NULL);

	//FIXME: WIP, may be some errors
	if (eval->type != EVAL_NUM)
		return FALSE;
	else if (eval->value == 0)
		return TRUE;
	else
		return FALSE;
}

eval_t *
eval_process_op(eval_t *left, eval_t *right, opcode_t opcode)
{
	assert(left != NULL && right != NULL);

	eval_t *ev;
	int l, r, res;
	
	//FIXME
	if (left->type != EVAL_NUM)
		return NULL;
	if (right->type != EVAL_NUM)
		return NULL;
	
	l = left->value;
	r = right->value;

	switch(opcode) {
	case OP_MUL:
		res = l * r;
		break;
	case OP_DIV:
		if (r == 0) {
			print_warn("divide by zero\n");
			return NULL;
		}
		res = l / r;
		break;
	case OP_PLUS:
		res = l + r;
		break;
	case OP_MINUS:
		res = l - r;
		break;
	case OP_EQ:
		res = (l == r);
		break;
	case OP_NEQ:
		res = (l != r);
		break;
	case OP_GR:
		res = (l > r);
		break;
	case OP_LO:
		res = (l < r);
		break;
	case OP_GE:
		res = (l >= r);
		break;
	case OP_LE:
		res = (l <= r);
		break;
	case OP_L_AND:
		res = (l && r);
		break;
	case OP_L_OR:
		res = (l || r);
		break;
	case OP_B_OR:
		res = (l | r);
		break;
	case OP_B_XOR:
		res = (l ^ r);
		break;
	case OP_B_AND:
		res = (l & r);
		break;
	case OP_SHR:
		res = l >> r;
		break;
	case OP_SHL:
		res = l << r;
		break;
	default:
		print_warn_and_die("unsupported tock recognised when eval\n");
	}
	ev = eval_num_new(res);

	return ev;
}

/* 
 * Print array 
 * debug function
 * later need to rewrite or replace
 */
void
eval_print_arr(arr_t *arr)
{
	int i, n, val;
	int *index;

	/*/debug{
	D(printf("%d dims\n", arr->dims));
 	for (i = 0; i < arr->dims; i++)
		D(printf("%d) len = %d\n", i, arr->len[i]));
	//}debug
	*/
	n = 1;

	for (i = 0; i < arr->dims; i++)
		n *= arr->len[i];
	
	//D(printf("n = %d\n", n));
	
	index = malloc_or_die(sizeof(*index) * arr->dims);
	memset(index, 0, sizeof(*index) * arr->dims);
	
	
	for (i = 0; i < arr->dims; i++)
		printf("{");

	while (1) {

		while (index[arr->dims - 1] < arr->len[arr->dims - 1]) {
			arr_get_item(arr, index, &val);
			printf(" %d,", val);
			index[arr->dims - 1]++;
		}

		index[arr->dims - 1] = 0;
	
		i = arr->dims - 2;
		while (i >= 0) {
			printf("}");
			index[i]++;
			if (index[i] >= arr->len[i]) {
				index[i] = index[i] % arr->len[i];
				i--;
			} else {
				break;
			}
		}
		if (i < 0)
			break;
		printf("{");
	}
	printf("}\n");

	ufree(index);
}


ret_t
eval_print_val(eval_t *eval)
{
	int value;

	if (eval == NULL)
		return ret_err;	

	switch(eval->type) {
	case EVAL_NUM:
		value = eval->value;
		printf("%d\n", value);
		break;
	case EVAL_ARR:
		eval_print_arr(eval->arr);	
		break;
	default:
		print_warn_and_die("INTERNAL ERROR: cant get value\n");
	}

	return ret_ok; 
}

