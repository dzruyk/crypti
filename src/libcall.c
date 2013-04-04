#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "crypt_hashes.h"
#include "eval.h"
#include "libcall.h"
#include "log.h"
#include "macros.h"
#include "variable.h"
#include "var_op.h"

int
libcall_print(id_item_t **args, int *rettypes, void **retvals)
{
	assert(args != NULL && args[0] != NULL);

	id_item_t *current;
	str_t *str;
	struct variable *var;

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
libcall_sum(id_item_t **args, int *rettypes, void **retvals)
{
	assert(args != NULL && args[0] != NULL);

	//id_item_t *current;

	//FIXME: stub, wanna implement variable length arguments funtions
	
	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	return 0;
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
	id_item_t *arg1;
	arr_item_t *aitem;
	arr_iterate_t *iter;
	arr_t *arr;
	struct variable *tmp;

	struct variable *min, *max;

	assert(args != NULL && args[0] != NULL);

	DEBUG(LOG_DEFAULT, "arr_min_max executed\n");

	arg1 = args[0];

	if (arg1->type != ID_ARR) {
		print_warn("error libcall_arr_min_max: array expected\n");
		return 1;
	}

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
	id_item_t *arg1, *arg2, *arg3;
	struct variable *res;
	int ret;

	assert(args != NULL && args[0] != NULL);

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	arg1 = args[0];
	arg2 = args[1];
	arg3 = args[2];
	if (arg1->type != ID_VAR ||
	    arg2->type != ID_VAR ||
	    arg3->type != ID_VAR) {
		print_warn("error libcall_subs: string expected\n");
		return 1;
	}
	res = xmalloc(sizeof(*res));
	var_init(res);
	
	ret = varop_str_sub(res, arg1->var, arg2->var, arg3->var);
	if (ret != 0)
		goto err;

	rettypes[0] = ID_VAR;
	retvals[0] = res;
	
	return 0;
err:
	var_clear(res);
	return 1;
}

int
libcall_subocts(id_item_t **args, int *rettypes, void **retvals)
{
	id_item_t *arg1, *arg2, *arg3;
	struct variable *res;
	int ret;

	assert(args != NULL && args[0] != NULL);

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	arg1 = args[0];
	arg2 = args[1];
	arg3 = args[2];
	if (arg1->type != ID_VAR ||
	    arg2->type != ID_VAR ||
	    arg3->type != ID_VAR) {
		print_warn("error libcall_subocts: string expected\n");
		return 1;
	}
	res = xmalloc(sizeof(*res));
	var_init(res);
	
	ret = varop_octstr_sub(res, arg1->var, arg2->var, arg3->var);
	if (ret != 0)
		goto err;

	rettypes[0] = ID_VAR;
	retvals[0] = res;
	
	return 0;
err:
	var_clear(res);
	return 1;
}

/*
int
libcall_del(id_item_t **args, int *rettypes, void **retvals)
{
	assert(args != NULL && args[0] != NULL);

	id_item_t *current;
	id_item_t *tmp;
	ret_t ret;

	current = args[0];

	//FIXME: rly need reserve some name?
	if (strcmp(current->name, "") == 0) {
		print_warn("cant delete, its not variable\n");
		return 1;
	}
	
	tmp = id_table_lookup_all(current->name);
	
	if (tmp == NULL)
		SHOULDNT_REACH();

	ret = id_table_remove(current->name);
	if (ret != ret_ok) {
		print_warn("symbol %s undefined\n", current->name);
		return 1;
	}
	
	args[0] = NULL;

	rettypes[0] = ID_UNKNOWN;
	retvals[0] = NULL;

	return 0;
}
*/

#define libcall_hash_generic(hash_name)	do {				\
	id_item_t *arg1;						\
	struct variable *res;						\
	octstr_t *dst, *src;						\
									\
	assert(args != NULL && args[0] != NULL);			\
									\
	arg1 = args[0];							\
	if (arg1->type != ID_VAR) {					\
		print_warn("error libcall " #hash_name			\
		": string expected\n");					\
		return 1;						\
	}								\
									\
	res = xmalloc(sizeof(*res));					\
	var_init(res);							\
									\
	src = var_cast_to_octstr(arg1->var);				\
	dst = var_octstr_ptr(res);					\
									\
	octstr_ ## hash_name (dst, src);						\
									\
	var_force_type(res, VAR_OCTSTRING);				\
									\
	rettypes[0] = ID_VAR;						\
	retvals[0] = res;						\
} while(0)

/* Crypto hashes (simple) */
int
libcall_md5(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(md5);

	return 0;
}

int
libcall_sha1(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(sha1);

	return 0;
}

int
libcall_sha256(id_item_t **args, int *rettypes, void **retvals)
{
	libcall_hash_generic(sha256);

	return 0;
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
	if (arg1->type != ID_VAR) {					\
		print_warn("error libcall_"				\
		    #hash_type "_init: string expected");		\
		return 1;						\
	}								\
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
	if (arg1->type != ID_VAR) {					\
		print_warn("error libcall_" # hash_type			\
		    "_init: string expected");				\
		return 1;						\
	}								\
	if (arg2->type != ID_VAR) {					\
		print_warn("error libcall_" # hash_type			\
		    "_init: string expected");				\
		return 1;						\
	}								\
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
	if (arg1->type != ID_VAR) {					\
		print_warn("error libcall_"# hash_type			\
		    "_init: string expected");				\
		return 1;						\
	}								\
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

