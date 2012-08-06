#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "common.h"
#include "list.h"

//FIXME: return value?

//FIXME STUB
typedef void (*lib_handler_t)(void);

typedef struct {
	char *name;
	int nargs;
	struct list *args;
	int is_lib;
	lib_handler_t *handler;
	void *body;
} func_t;

void function_table_init();

ret_t function_table_insert(func_t *item);

func_t * function_table_lookup(char *name);

void function_table_destroy();

func_t *func_new(char *name);

//FIXME: REWRITE ME. function.c have function_table_destroy_cb
ret_t func_table_delete(func_t *func);

void func_set_args(func_t *func, struct list *args);

void func_set_body(func_t *func, void *body);

#endif

