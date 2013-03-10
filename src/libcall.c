#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "eval.h"
#include "libcall.h"
#include "macros.h"
#include "variable.h"
#include "var_op.h"

int
libcall_print(id_item_t **args, int *rettype, void **retval)
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

	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}

int
libcall_sum(id_item_t **args, int *rettype, void **retval)
{
	assert(args != NULL && args[0] != NULL);

	//id_item_t *current;

	//FIXME: stub, wanna implement variable argue funtion
	
	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}

int
libcall_type(id_item_t **args, int *rettype, void **retval)
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

	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}

int
libcall_subs(id_item_t **args, int *rettype, void **retval)
{
	id_item_t *arg1, *arg2, *arg3;
	struct variable *res;
	int ret;

	assert(args != NULL && args[0] != NULL);

	*rettype = ID_UNKNOWN;
	*retval = NULL;

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

	*rettype = ID_VAR;
	*retval = res;
	
	return 0;
err:
	var_clear(res);
	return 1;
}

int
libcall_subocts(id_item_t **args, int *rettype, void **retval)
{
	id_item_t *arg1, *arg2, *arg3;
	struct variable *res;
	int ret;

	assert(args != NULL && args[0] != NULL);

	*rettype = ID_UNKNOWN;
	*retval = NULL;

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

	*rettype = ID_VAR;
	*retval = res;
	
	return 0;
err:
	var_clear(res);
	return 1;
}

/*
int
libcall_del(id_item_t **args, int *rettype, void **retval)
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

	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}
*/
