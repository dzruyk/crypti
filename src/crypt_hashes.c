#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "crypt_hashes.h"
#include "defaults.h"
#include "hash.h"
#include "macros.h"

#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "crypto/whirlpool.h"

#define MAX_N_CTX 32

struct ctx_table {
	struct hash_table *hash;
	int count;
};

static struct ctx_table *ctx_table = NULL;

struct hash_ctx {
	char *name;
	int type;
	void *ctx;
};

enum {
	CTX_TYPE_MD5,
	CTX_TYPE_SHA1,
	CTX_TYPE_SHA256,
	CTX_TYPE_WHIRLPOOL,
};

void
hash_ctx_table_init()
{
	if (ctx_table != NULL)
		error(1, "ctx table already initialisated!\n");

	ctx_table = xmalloc(sizeof(*ctx_table));

	ctx_table->hash = hash_table_new(INITIAL_SZ,
	    (hash_callback_t )default_hash_cb,
	    (hash_compare_t )default_hash_compare);
	ctx_table->count = 0;

	if (ctx_table->hash == NULL)
		error(1, "error at table table creation\n");
}

static void
hash_ctx_destroy_cb(struct hash_ctx *ctx)
{
	ufree(ctx->name);
	ufree(ctx->ctx);
	ufree(ctx);
}

void
hash_ctx_table_destroy()
{
	void *key, *data;
	struct hash_table_iter *iter;

	iter = hash_table_iterate_init(ctx_table->hash);

	while (hash_table_iterate(iter, &key, &data) != FALSE)
		hash_ctx_destroy_cb((struct hash_ctx *)data);

	hash_table_iterate_deinit(&iter);

	hash_table_destroy(&ctx_table->hash);

	ufree(ctx_table);
	ctx_table = NULL;
}

static ret_t
hash_ctx_table_insert(struct hash_ctx *item)
{
	int res;

	assert(item != NULL);

	if (ctx_table->count + 1 > MAX_N_CTX) {
		print_warn("to many ctx created, limit %d\n", MAX_N_CTX);
		return ret_err;
	}

	res = hash_table_insert_unique(ctx_table->hash, item->name, item);
	if (res == ret_out_of_memory)
		error(1, "error at func table insertion\n");
	else if (res == ret_entry_exists)
		error(1, "internal error, entry exists\n");

	ctx_table->count++;

	return ret_ok;
}

static struct hash_ctx *
hash_ctx_table_lookup(char *name)
{
	struct hash_ctx *res;

	assert(name != NULL);

	if (hash_table_lookup(ctx_table->hash, name, (void **)&res) != ret_ok)
		return NULL;
	else
		return res;
}

static struct hash_ctx *
hash_ctx_new(char *name, int type, void *ctx)
{
	struct hash_ctx *item;

	item = xmalloc(sizeof(*item));
	memset(item, 0, sizeof(*item));

	item->name = strdup_or_die(name);
	item->type = type;
	item->ctx = ctx;

	return item;
}

static ret_t
hash_ctx_delete(struct hash_ctx *item)
{
	int ret;

	assert(item != NULL);

	ret = hash_table_remove(ctx_table->hash, item->name);
	if (ret == FALSE)
		return ret_err;

	hash_ctx_destroy_cb(item);
	ctx_table->count--;

	return ret_ok;
}

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

void
octstr_whirlpool(octstr_t *dst, octstr_t *src)
{
	generic_hash(dst, src, whirlpool, 64);
}

#define generic_hash_init(id, hash_name, ctx_type)	do {	\
	struct hash_ctx *h;					\
	struct hash_name ## _context *ctx;			\
	char *p;						\
								\
	p = str_ptr(id);					\
								\
	if (hash_ctx_table_lookup(p) != NULL) {			\
		print_warn(					\
		    "ctx with name %s already created\n", p);	\
		return 1;					\
	}							\
								\
	ctx = xmalloc(sizeof(*ctx));				\
	hash_name ## _context_init(ctx);			\
								\
	h = hash_ctx_new(p, ctx_type, ctx);			\
								\
	hash_ctx_table_insert(h);				\
								\
	return 0;						\
} while (0)

#define generic_hash_update(id, data, hash_name, ctx_type) do {	\
	struct hash_ctx *ctx;					\
	char *s;						\
	void *p;						\
	int len;						\
								\
	s = str_ptr(id);					\
								\
	ctx = hash_ctx_table_lookup(s);				\
	if (ctx == NULL) {					\
		print_warn(					\
		    "can't find ctx with name %s\n", s);	\
		return 1;					\
	}							\
								\
	if (ctx->type != ctx_type) {				\
		print_warn("invalid ctx type\n");		\
		return 1;					\
	}							\
								\
	p = octstr_ptr(data);					\
	len = octstr_len(data);					\
								\
	hash_name ## _update(ctx->ctx, p, len);			\
								\
	return 0;						\
} while (0)

#define generic_hash_finalize(id, out, hash_name, ctx_type, hash_len) \
do {								\
	struct hash_ctx *ctx;					\
	char *s;						\
	unsigned char d[hash_len];				\
								\
	s = str_ptr(id);					\
	ctx = hash_ctx_table_lookup(s);				\
	if (ctx == NULL) {					\
		print_warn("can't find ctx with name %s\n", s);	\
		return 1;					\
	}							\
								\
	if (ctx->type != ctx_type) {				\
		print_warn("invalid ctx type\n");		\
		return 1;					\
	}							\
								\
	hash_name ## _final(ctx->ctx, d);			\
								\
	octstr_reset(out);					\
	octstr_append_n(out, d, sizeof(d));			\
								\
	if (hash_ctx_delete(ctx) != ret_ok)			\
		return 1;					\
								\
	return 0;						\
} while (0)

int
octstr_md5_init(str_t *id)
{
	generic_hash_init(id, md5, CTX_TYPE_MD5);
}

int
octstr_md5_update(str_t *id, octstr_t *data)
{
	generic_hash_update(id, data, md5, CTX_TYPE_MD5);
}

int
octstr_md5_finalize(str_t *id, octstr_t *out)
{
	generic_hash_finalize(id, out, md5, CTX_TYPE_MD5, 16);
}

int
octstr_sha1_init(str_t *id)
{
	generic_hash_init(id, sha1, CTX_TYPE_SHA1);
}

int
octstr_sha1_update(str_t *id, octstr_t *data)
{
	generic_hash_update(id, data, sha1, CTX_TYPE_SHA1);
}

int
octstr_sha1_finalize(str_t *id, octstr_t *out)
{
	generic_hash_finalize(id, out, sha1, CTX_TYPE_SHA1, 20);

}

int
octstr_sha256_init(str_t *id)
{
	generic_hash_init(id, sha256, CTX_TYPE_SHA256);
}

int
octstr_sha256_update(str_t *id, octstr_t *data)
{
	generic_hash_update(id, data, sha256, CTX_TYPE_SHA256);
}

int
octstr_sha256_finalize(str_t *id, octstr_t *out)
{
	generic_hash_finalize(id, out, sha256, CTX_TYPE_SHA256, 32);
}

int
octstr_whirlpool_init(str_t *id)
{
	generic_hash_init(id, whirlpool, CTX_TYPE_WHIRLPOOL);
}

int
octstr_whirlpool_update(str_t *id, octstr_t *data)
{
	generic_hash_update(id, data, whirlpool, CTX_TYPE_WHIRLPOOL);
}

int
octstr_whirlpool_finalize(str_t *id, octstr_t *out)
{
	generic_hash_finalize(id, out, whirlpool, CTX_TYPE_WHIRLPOOL, 64);
}

