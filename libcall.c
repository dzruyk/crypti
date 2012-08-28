#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "eval.h"
#include "libcall.h"

int
libcall_print(id_item_t **argues, int *rettype, void **retval)
{
	assert(argues != NULL && argues[0] != NULL);

	id_item_t *arg;

	arg = argues[0];
	
	switch (arg->type) {
	case ID_NUM:
		printf("%d\n", arg->value);
		break;
	case ID_ARR:
		printf("array print: WIP\n");
		break;
	default:
		print_warn_and_die("something wrong\n");
	}

	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}

int
libcall_sum(id_item_t **argues, int *rettype, void **retval)
{
	assert(argues != NULL && argues[0] != NULL);

	id_item_t *arg;

	//FIXME: stub, wanna implement variable argue funtion
	
	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}

int
libcall_type(id_item_t **argues, int *rettype, void **retval)
{
	assert(argues != NULL && argues[0] != NULL);

	id_item_t *arg;
	
	arg = argues[0];

	switch (arg->type) {
	case ID_NUM:
		printf("<type num>\n");
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
libcall_del(id_item_t **argues, int *rettype, void **retval)
{
	assert(argues != NULL && argues[0] != NULL);

	id_item_t *arg;
	ret_t ret;

	arg = argues[0];

	//FIXME: rly need reserve some name?
	if (strcmp(arg->name, "") == 0) {
		print_warn("cant delete, its not variable\n");
		return 1;
	}
	
	//then delete

	ret = id_table_remove(arg->name);
	if (ret != ret_ok) {
		print_warn("symbol %s undefined\n", arg->name);
		return 1;
	}
	
	argues[0] = NULL;

	*rettype = ID_UNKNOWN;
	*retval = NULL;

	return 0;
}

