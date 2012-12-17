#ifndef __EVAL_H__
#define __EVAL_H__

#include "array.h"
#include "id_table.h"
#include "syn_tree.h"

typedef enum {
	EVAL_VAR,
	EVAL_ARR,
} eval_type_t;


typedef struct {
	eval_type_t type;
	union {
		struct variable *var;
		arr_t *arr;
	};
} eval_t;

/*
 * Create new num eval.
 */
eval_t *eval_var_new();

/*
 * Create new array eval.
 */
eval_t *eval_arr_new(arr_t *arr);

/*
 * Free passed eval_t.
 * WARNING: You must manualy free eval payload.
 */
void eval_free(eval_t *eval);


/*
 * Check passed eval:
 * return TRUE if eval is zero;
 * return FALSE otherwise.
 */
boolean_t eval_is_zero(eval_t *eval);

/*
 * Process unary operation, returns new eval_t.
 */
eval_t *eval_process_unary(eval_t *ev, opcode_t opcode);

/*
 * Process expression, returns result in new eval_t.
 */
eval_t *eval_process_op(eval_t *left, eval_t *right, opcode_t opcode);

/*
 * Prints passed eval.
 */
ret_t eval_print_val(eval_t *eval);

#endif

