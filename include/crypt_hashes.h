#ifndef __CRYPT_HASHES_H__
#define __CRYPT_HASHES_H__

void octstr_md5(octstr_t *dst, ocstr_t *src);
void octstr_sha1(octstr_t *dst, ocstr_t *src);
void octstr_sha256(octstr_t *dst, ocstr_t *src);

#endif
