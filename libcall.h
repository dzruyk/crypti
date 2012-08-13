#ifndef __LIBCALL_H__
#define __LIBCALL_H__

#include "id_table.h"

int libcall_print(id_item_t **argues, int *rettype, void **retval);

int libcall_sum(id_item_t **argues, int *rettype, void **retval);

#endif
