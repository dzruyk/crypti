#include <assert.h>
#include <stdio.h>

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
libcall_del(id_item_t **argues, int *rettype, void **retval)
{
	assert(argues != NULL && argues[0] != NULL);

	id_item_t *arg, *item;

	arg = argues[0];
/* not work now
	item = id_table_lookup_all(arg->name);
	if (item == NULL) {
		print_warn("symbol %s undefined");


	*rettype = ID_UNKNOWN;
	*retval = NULL;
*/
	return 0;
}
