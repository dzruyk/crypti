#include <stdlib.h>

#include "crypt_hashes.h"

#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"


#define generic_hash(dst, src, hash_name, hash_len) 	do {	\
	struct hash_name ## _context ctx;			\
	unsigned char d[hash_len];				\
	unsigned char *p;					\
	size_t len;						\
								\
	hash_name ## _context_init(&ctx);			\
								\
	len = octstr_len(src);					\
	p = octstr_ptr(src);					\
								\
	hash_name ## _update(&ctx, p, len);			\
								\
	hash_name ## _final(&ctx, d);				\
								\
	octstr_reset(dst);					\
	octstr_append_n(dst, d, sizeof(d));			\
} while(0)


void
octstr_md5(octstr_t *dst, octstr_t *src)
{
	generic_hash(dst, src, md5, 16);
/*
	struct md5_context ctx;
	unsigned char d[16];
	unsigned char *p;
	size_t len;

	md5_context_init(&ctx);

	len = octstr_len(src);
	p = octstr_ptr(src);

	md5_update(&ctx, p, len);

	md5_final(&ctx, d);

	octstr_reset(dst);
	octstr_append_n(dst, d, sizeof(d));
*/
}

void
octstr_sha1(octstr_t *dst, octstr_t *src)
{
	generic_hash(dst, src, sha1, 20);
}


void
octstr_sha256(octstr_t *dst, octstr_t *src)
{
	generic_hash(dst, src, sha256, 32);
}

