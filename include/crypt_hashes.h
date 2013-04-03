#ifndef __CRYPT_HASHES_H__
#define __CRYPT_HASHES_H__

#include "common.h"
#include "octstr.h"
#include "str.h"

void hash_ctx_table_init();
void hash_ctx_table_destroy();

void octstr_md5(octstr_t *dst, octstr_t *src);
void octstr_sha1(octstr_t *dst, octstr_t *src);
void octstr_sha256(octstr_t *dst, octstr_t *src);

int octstr_md5_init(str_t *id);
int octstr_md5_update(str_t *id, octstr_t *data);
int octstr_md5_finalize(str_t *id, octstr_t *out);

#endif
