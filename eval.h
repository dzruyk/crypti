#ifndef __EVAL_H__
#define __EVAL_H__

#include "array.h"
#include "id_table.h"
#include "syn_tree.h"

typedef enum {
	EVAL_NUM,
	EVAL_FUNC,
	EVAL_ARR,
} eval_type_t;


typedef struct {
	eval_type_t type;
	union {
		int value;
		arr_t *arr;
	};
} eval_t;

/*
 * create new num eval
 */
eval_t *eval_num_new();

/*
 * create new array eval
 */
eval_t *eval_arr_new(arr_t *arr);

/*
 * free passed eval_t
 * WARNING: you must manualy free eval payload
 */
void eval_free(eval_t *eval);

/*
 * process expression, returns result
 */
eval_t *eval_process_op(eval_t *left, eval_t *right, opcode_t opcode);

/*
 * prints passed eval
 */
ret_t eval_print_val(eval_t *eval);

#endif

