#ifndef __LIBCALL_H__
#define __LIBCALL_H__

#include "id_table.h"

int libcall_print(id_item_t **args, int *rettypes, void **retvals);

int libcall_sum(id_item_t **args, int *rettypes, void **retvals);

int libcall_type(id_item_t **args, int *rettypes, void **retvals);

int libcall_arr_min_max(id_item_t **args, int *rettypes, void **retvals);

int libcall_subs(id_item_t **args, int *rettypes, void **retvals);

int libcall_subocts(id_item_t **args, int *rettypes, void **retvals);

//int libcall_del(id_item_t **args, int *rettypes, void **retvals);

/* Crypto hashes (simple) */
int libcall_md5(id_item_t **args, int *rettypes, void **retvals);
int libcall_sha1(id_item_t **args, int *rettypes, void **retvals);
int libcall_sha256(id_item_t **args, int *rettypes, void **retvals);

/* Crypto hashes (full) */
int libcall_md5_init(id_item_t **args, int *rettypes, void **retvals);
int libcall_md5_update(id_item_t **args, int *rettypes, void **retvals);
int libcall_md5_finalize(id_item_t **args, int *rettypes, void **retvals);

int libcall_sha1_init(id_item_t **args, int *rettypes, void **retvals);
int libcall_sha1_update(id_item_t **args, int *rettypes, void **retvals);
int libcall_sha1_finalize(id_item_t **args, int *rettypes, void **retvals);

int libcall_sha256_init(id_item_t **args, int *rettypes, void **retvals);
int libcall_sha256_update(id_item_t **args, int *rettypes, void **retvals);
int libcall_sha256_finalize(id_item_t **args, int *rettypes, void **retvals);
#endif
