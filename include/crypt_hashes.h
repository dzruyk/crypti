#ifndef __CRYPT_HASHES_H__
#define __CRYPT_HASHES_H__

#include "octstr.h"

void octstr_md5(octstr_t *dst, octstr_t *src);
void octstr_sha1(octstr_t *dst, octstr_t *src);
void octstr_sha256(octstr_t *dst, octstr_t *src);

#endif
