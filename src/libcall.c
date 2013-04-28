#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "climits.h"
#include "crypt_hashes.h"
#include "eval.h"
#include "libcall.h"
#include "log.h"
#include "macros.h"
#include "variable.h"
#include "var_op.h"
#include "var_print.h"

#define CHECK_TYPE(arg, expected)			do {	\
	if (arg->type != expected) {				\
		print_warn("error: %s expected\n", 		\
		id_type_2_str(expected));			\
		return 1;					\
	}							\
} while(0)

#define FUNC_1_ARG(f) ((int (*)(struct  variable *, struct  variable *))(f))
#define FUNC_2_ARG(f) ((int (*)(struct  variable *, struct  variable *, \
    struct  variable *))(f))
#define FUNC_3_ARG(f) ((int (*)(struct  variable *, struct  variable *, \
    struct  variable *, struct  variable *))(f))

#define exec_with_err_handler(args, rettypes, retvals, nargs, func, HANDLER)	\
do {								\
	struct variable *vars[MAXFARGS];			\
	struct variable *res;					\
	void *f = func;						\
	int i, ret;						\
								\
	assert(args != NULL);					\
								\
	for (i = 0; i < nargs; i++) {				\
		assert(args[i] != NULL);			\
		CHECK_TYPE(args[i], ID_VAR);			\
		vars[i] = args[i]->var;				\
	}							\
								\
	rettypes[0] = ID_UNKNOWN;				\
	retvals[0] = NULL;					\
								\
	res = xmalloc(sizeof(*res));				\
	var_init(res);						\
								\
	if (nargs == 1) {					\
		ret = FUNC_1_ARG(f)(res, vars[0]);		\
	} else if (nargs == 2) {				\
		ret = FUNC_2_ARG(f)(res, vars[0], vars[1]);	\
	} else if (nargs == 3) {				\
		ret = FUNC_3_ARG(f)(res, vars[0], vars[1], 	\
		    vars[2]);					\
	} else {						\
		SHOULDNT_REACH();				\
	}							\
								\
	if (ret != 0)						\
		HANDLER						\
								\
	rettypes[0] = ID_VAR;					\
	retvals[0] = res;					\
								\
	return 0;						\
} while (0)

#define ACT_SET_MINUS_ONE					\
{								\
	mpl_int *tmp;						\
	tmp = var_bignum_ptr(res);				\
	mpl_set_sint(tmp, -1);					\
	var_force_type(res, VAR_BIGNUM);			\
}

#define ACT_RET_ERR						\
{								\
	var_clear(res);						\
	return 1;						\
}

int
libcall_print(id_item_t **args, int *rettypes, void **retvals)
{
	id_item_t *current;
	str_t *str;
	struct variable *var;

	assert(args != NULL && args[0] != NULL);

	current = args[0];
	
	switch (current->type) {
	case ID_VAR:
		var = current->var;
		str = var_cast_to_str(var);

		printf("%s\n", str_ptr(str));
		break;
	case ID_ARR:
		arr_print(current->arr);
		break;
	default:
		error(1, "something wrong\n");
	}

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	return 0;
}

int
libcall_printf(id_item_t **args, int *rettypes, void **retvals)
{
	id_item_t *fmt;
	struct variable *vars[MAXFARGS];
	int i;

	if (*args == NULL) {
		print_warn("printf need at least 1 argument\n");
		return 1;
	}
	fmt = *args++;
	CHECK_TYPE(fmt, ID_VAR);
	
	for (i = 0; args[i] != NULL; i++) {
		assert(i < MAXFARGS);
		//printf can process only variables now.
		CHECK_TYPE(args[i], ID_VAR);
		vars[i] = args[i]->var;
	}

	var_print_formatted(fmt->var, vars, i);

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;
	
	return 0;
}

int
libcall_sum(id_item_t **args, int *rettypes, void **retvals)
{
	id_item_t *arg;
	struct variable *res;

	assert(args != NULL);

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	res = xmalloc(sizeof(*res));
	var_init(res);
	var_set_zero(res);
	
	while (*args != NULL) {
		arg = *args;
		if (arg->type != ID_VAR) {
			print_warn("error: %s ID_VAR\n",
			id_type_2_str(ID_VAR));
			goto err;
		}
		
		if (varop_add(res, res, arg->var) != 0)
			goto err;
		args++;
	}

	rettypes[0] = ID_VAR;
	retvals[0] = res;

	return 0;
err:
	var_clear(res);
	return 1;
}

int
libcall_type(id_item_t **args, int *rettypes, void **retvals)
{
	id_item_t *current;
	
	assert(args != NULL && args[0] != NULL);

	current = args[0];

	switch (current->type) {
	case ID_VAR:
		printf("<type var>\n");
		break;
	case ID_ARR:
		printf("<type arr>\n");
		break;
	default:
		printf("<type unknown>\n");
		break;
	}

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	return 0;
}

int
libcall_arr_min_max(id_item_t **args, int *rettypes, void **retvals)
{
	arr_item_t *aitem;
	arr_iterate_t *iter;
	arr_t *arr;
	id_item_t *arg1;
	struct variable *tmp;
	struct variable *min, *max;

	assert(args != NULL && args[0] != NULL);

	DEBUG(LOG_DEFAULT, "arr_min_max executed\n");

	arg1 = args[0];

	CHECK_TYPE(arg1, ID_ARR);

	arr = arg1->arr;

	if (arr->nitems == 0) {
		print_warn("array is empty\n");
		return 1;
	}

	min = xmalloc(sizeof(*min));
	max = xmalloc(sizeof(*max));
	var_initv(min, max, NULL);

	iter = array_iterate_new(arr);
	
	array_iterate(iter, &aitem);
	var_copy(min, aitem->var);
	var_copy(max, aitem->var);

	while (array_iterate(iter, &aitem)) {
		tmp = aitem->var;
		if (varop_cmp(min, tmp) == -1)
			var_copy(min, tmp);
		if (varop_cmp(max, tmp) == 1)
			var_copy(max, tmp);
	}
	
	array_iterate_free(iter);

	rettypes[0] = ID_VAR;
	rettypes[1] = ID_VAR;
	retvals[0] = min;
	retvals[1] = max;

	return 0;
}

int
libcall_subs(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 3,
	    varop_str_sub, ACT_RET_ERR);
}

int
libcall_subocts(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 3,
	    varop_octstr_sub, ACT_RET_ERR);
}

int
libcall_len(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 1,
	    varop_str_len, ACT_RET_ERR);
}

int
libcall_size(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 1,
	    varop_octstr_len, ACT_RET_ERR);
}

int
libcall_lpad(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 3,
	    varop_lpad, ACT_RET_ERR);
}

int
libcall_rpad(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 3,
	    varop_rpad, ACT_RET_ERR);
}

int
libcall_randint(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 2,
	   varop_rand_int, ACT_RET_ERR);
}

int
libcall_gcd(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 2,
	    varop_gcd, ACT_SET_MINUS_ONE);
}

int
libcall_mod_inv(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 2,
	    varop_mod_inv, ACT_SET_MINUS_ONE);
}


int
libcall_mod_exp(id_item_t **args, int *rettypes, void **retvals)
{
	exec_with_err_handler(args, rettypes, retvals, 3,
	    varop_mod_exp, ACT_RET_ERR);
}


#define libcall_hash_generic(hash_name)	do {				\
	id_item_t *arg1;						\
	struct variable *res;						\
	octstr_t *dst, *src;						\
									\
	assert(args != NULL && args[0] != NULL);			\
									\
	arg1 = args[0];							\
	CHECK_TYPE(arg1, ID_VAR);					\
									\
	res = xmalloc(sizeof(*res));					\
	var_init(res);							\
									\
	src = var_cast_to_octstr(arg1->var);				\
	dst = var_octstr_ptr(res);					\
									\
	octstr_ ## hash_name (dst, src);				\
									\
	var_force_type(res, VAR_OCTSTRING);				\
									\
	rettypes[0] = ID_VAR;						\
	retvals[0] = res;						\
									\
	return 0;							\
} while(0)

/* Crypto hashes (simple) */
int
libcall_md5(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(md5);
}

int
libcall_sha1(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(sha1);
}

int
libcall_sha256(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(sha256);
}

int
libcall_whirlpool(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(whirlpool);
}

/* Crypto hashes (full) */

#define libcall_hash_generic_init(args, rettypes, retvals, hash_type) 	\
do {									\
	id_item_t *arg1;						\
	struct variable *res;						\
	mpl_int *st;							\
	str_t *id;							\
	int ret;							\
									\
	arg1 = args[0];							\
									\
	CHECK_TYPE(arg1, ID_VAR);					\
									\
	res = xmalloc(sizeof(*res));					\
	var_init(res);							\
									\
	st = var_bignum_ptr(res);					\
									\
	id = var_cast_to_str(arg1->var);				\
									\
	ret = octstr_ ## hash_type ## _init(id);			\
	if (ret != 0)							\
		mpl_set_one(st);					\
	else								\
		mpl_set_sint(st, 0);					\
									\
	var_force_type(res, VAR_BIGNUM);				\
									\
	rettypes[0] = ID_VAR;						\
	retvals[0] = res;						\
									\
	return 0;							\
} while (0)

#define libcall_hash_generic_update(args, rettypes, retvals, hash_type) \
do {									\
	id_item_t *arg1, *arg2;						\
	str_t *id;							\
	octstr_t *chunk;						\
									\
	arg1 = args[0];							\
	arg2 = args[1];							\
									\
	CHECK_TYPE(arg1, ID_VAR);					\
	CHECK_TYPE(arg2, ID_VAR);					\
									\
	id = var_cast_to_str(arg1->var);				\
	chunk = var_cast_to_octstr(arg2->var);				\
									\
	octstr_ ## hash_type ## _update(id, chunk);			\
									\
	rettypes[0] = ID_UNKNOWN;					\
	retvals[0] = NULL;						\
									\
	return 0;							\
} while (0)


#define libcall_hash_generic_finalize(args, rettypes, retvals, hash_type) \
do {									\
	struct variable *res;						\
	id_item_t *arg1;						\
	str_t *id;							\
	octstr_t *out;							\
									\
	arg1 = args[0];							\
									\
	CHECK_TYPE(arg1, ID_VAR);					\
									\
	res = xmalloc(sizeof(*res));					\
	var_init(res);							\
									\
	id = var_cast_to_str(arg1->var);				\
	out = var_octstr_ptr(res);					\
									\
	octstr_ ## hash_type ## _finalize(id, out);			\
									\
	var_force_type(res, VAR_OCTSTRING);				\
									\
	rettypes[0] = ID_VAR;						\
	retvals[0] = res;						\
									\
									\
	return 0;							\
									\
} while (0)

int
libcall_md5_init(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_init(args, rettypes, retvals, md5);
}

int
libcall_md5_update(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_update(args, rettypes, retvals, md5);
}

int
libcall_md5_finalize(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_finalize(args, rettypes, retvals, md5);
}


int
libcall_sha1_init(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_init(args, rettypes, retvals, sha1);
}

int
libcall_sha1_update(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_update(args, rettypes, retvals, sha1);
}

int
libcall_sha1_finalize(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_finalize(args, rettypes, retvals, sha1);
}


int
libcall_sha256_init(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_init(args, rettypes, retvals, sha256);
}

int
libcall_sha256_update(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_update(args, rettypes, retvals, sha256);
}

int
libcall_sha256_finalize(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_finalize(args, rettypes, retvals, sha256);
}


int
libcall_whirlpool_init(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_init(args, rettypes, retvals, whirlpool);
}

int
libcall_whirlpool_update(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_update(args, rettypes, retvals, whirlpool);
}

int
libcall_whirlpool_finalize(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic_finalize(args, rettypes, retvals, whirlpool);
}

