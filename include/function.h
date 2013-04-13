#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "common.h"
#include "id_table.h"
#include "syn_tree.h"

#define FUNC_VAR_ARGS -1

//FIXME STUB
typedef int (*libcall_handler_t)(id_item_t **args, int *rettypes, void **retvals);

typedef struct {
	char *name;
	char **args;
	int nargs;
	char **retargs;
	int nret;
	int is_lib;
	union {
		libcall_handler_t handler;
		void *body;
	};
} func_t;

void function_table_init();

ret_t function_table_insert(func_t *item);

func_t * function_table_lookup(char *name);

void function_table_destroy();

func_t *func_new(char *name);

//FIXME: REWRITE ME. function.c have function_table_destroy_cb
ret_t func_table_delete(func_t *func);

void func_set_retargs(func_t *func, char **retargs, int nret);
/*
 * add arguments to function
 * WARNING: multiple set_args can be possible memory leak
 */
void func_set_args(func_t *func, char **args, int nargs);

void func_set_body(func_t *func, void *body);

#endif

