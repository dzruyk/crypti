#include "macros.h"
#include "octstr.h"

#include "lib/crypto/md5.h"
#include "lib/crypto/sha1.h"
#include "lib/crypto/sha256.h"


void
octstr_md5(octstr_t *dst, ocstr_t *src)
{
	struct md5_context ctx;
	unsigned char d[16];
	unsigned char *p;

	md5_context_init(&ctx);

	len = octstr_len(src);
	p = octstr_ptr(src);

	md5_update(&ctx, p, len);

	md5_final(&ctx, d);

	octstr_reset(dst);
	octstr_append_n(dst, d, sizeof(d));
}

void
octstr_sha1(octstr_t *dst, ocstr_t *src)
{

}


void
octstr_sha256(octstr_t *dst, ocstr_t *src)
{

}

