#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "common.h"
#include "list.h"

//FIXME: return value?

//FIXME STUB
typedef void (*lib_handler_t)(void);

typedef struct {
	char *name;
	int narg;
	struct list *args;
	int is_lib;
	lib_handler_t *handler;
	void *id_table;
	void *body;
} func_t;

void function_table_init();

ret_t function_table_insert(func_t *item);

func_t * function_table_lookup(char *name);

void function_table_destroy();

func_t *func_new();

ret_t func_delete(func_t *func);


#endif

