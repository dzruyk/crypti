#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "common.h"
#include "log.h"
#include "macros.h"
#include <mpl.h>
#include "random.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"
#include "var_op.h"


/* bignum operations: */

typedef int (*bnum_op_func_t)(mpl_int *c, const mpl_int *a, const mpl_int *b);

static inline int
varop_bnum_op(struct variable *c, struct variable *a, struct variable *b, bnum_op_func_t mpl_func)
{
	int ret;
	mpl_int *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL && mpl_func != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	ret = mpl_func(cp, ap, bp);
	if (ret != MPL_OK) {
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);

	return 0;
}

/* Number operations: */
int
varop_add(struct variable *c, struct variable *a, struct variable *b)
{
	return varop_bnum_op(c, a, b, mpl_add);
}

int
varop_sub(struct variable *c, struct variable *a, struct variable *b)
{
	return varop_bnum_op(c, a, b, mpl_sub);
}

int
varop_mul(struct variable *c, struct variable *a, struct variable *b)
{
	return varop_bnum_op(c, a, b, mpl_mul);
}

int
varop_pow(struct variable *c, struct variable *a, struct variable *b)
{
	return varop_bnum_op(c, a, b, mpl_exp);
}

int
varop_gcd(struct variable *c, struct variable *a, struct variable *b)
{
	return varop_bnum_op(c, a, b, mpl_gcd);
}

int
varop_mod_inv(struct variable *c, struct variable *a, struct variable *b)
{
	return varop_bnum_op(c, a, b, mpl_mod_inv);
}

int
varop_div(struct variable *c, struct variable *a, struct variable *b)
{
	mpl_int *ap, *bp, *cp;
	int ret;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	ret = mpl_div(cp, NULL, ap, bp);
	if (ret != MPL_OK) {
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);

	return 0;
}

int
varop_mod(struct variable *c, struct variable *a, struct variable *b)
{
	mpl_int *ap, *bp, *cp;
	int ret;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	ret = mpl_div(NULL, cp, ap, bp);
	if (ret != MPL_OK) {
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);

	return 0;
}

int
varop_mod_exp(struct variable *res, struct variable *a, struct variable *y,
    struct variable *b)
{
	mpl_int *ap, *bp, *yp, *resp;
	int ret;

	assert(res != NULL && a != NULL && y != NULL && b != NULL);
	
	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	yp = var_cast_to_bignum(y);
	resp = var_bignum_ptr(res);

	ret = mpl_mod_exp(resp, ap, yp, bp);
	if (ret != MPL_OK) {
		return 1;
	}

	var_force_type(res, VAR_BIGNUM);

	return 0;
}

int
varop_or(struct variable *c, struct variable *a, struct variable *b)
{
	octstr_t *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);
	
	ap = var_cast_to_octstr(a);
	bp = var_cast_to_octstr(b);
	cp = var_octstr_ptr(c);

	octstr_or(cp, ap, bp);

	var_force_type(c, VAR_OCTSTRING);

	return 0;
}

int
varop_xor(struct variable *c, struct variable *a, struct variable *b)
{
	octstr_t *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);
	
	ap = var_cast_to_octstr(a);
	bp = var_cast_to_octstr(b);
	cp = var_octstr_ptr(c);

	octstr_xor(cp, ap, bp);

	var_force_type(c, VAR_OCTSTRING);

	return 0;
}

int
varop_and(struct variable *c, struct variable *a, struct variable *b)
{
	octstr_t *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);
	
	ap = var_cast_to_octstr(a);
	bp = var_cast_to_octstr(b);
	cp = var_octstr_ptr(c);

	octstr_and(cp, ap, bp);

	var_force_type(c, VAR_OCTSTRING);

	return 0;
}

int
varop_not(struct variable *res, struct variable *var)
{
	mpl_int *ap, *bp;

	assert(res != NULL && var != NULL);

	ap = var_cast_to_bignum(var);
	bp = var_bignum_ptr(res);

	if (mpl_iszero(ap))
		mpl_set_one(bp);
	else
		mpl_zero(bp);
	
	var_force_type(res, VAR_BIGNUM);

	return 0;
}

int
varop_neg(struct variable *res, struct variable *var)
{
	assert(res != NULL && var != NULL);

	mpl_int *dst, *src;

	src = var_cast_to_bignum(var);
	dst = var_bignum_ptr(res);
	
	mpl_copy(dst, src);

	dst->sign = !src->sign;

	var_force_type(res, VAR_BIGNUM);

	return 0;
}

int
varop_shl(struct variable *c, struct variable *a, struct variable *b)
{
	int n, ret;
	unsigned long shift;
	mpl_int *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	if (mpl_isneg(bp)) {
		DEBUG(LOG_DEFAULT, "negative shift integer\n");
		return 1;
	}

	n = mpl_nr_bits(bp);

	if (n > sizeof(int) * CHAR_BIT) {
		DEBUG(LOG_DEFAULT, "can't convert to int, overflow\n");
		return 1;
	}

	ret = mpl_to_uint(bp, &shift);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "can't convert to uint\n");
		return 1;
	}
	
	ret = mpl_copy(cp, ap);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "can't copy bnum\n");
		return 1;
	}

	ret = mpl_shl(cp, shift);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "can't shift left\n");
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);
	return 0;
}

int
varop_shr(struct variable *c, struct variable *a, struct variable *b)
{
	int n, ret;
	unsigned long shift;
	mpl_int *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	if (mpl_isneg(bp)) {
		DEBUG(LOG_DEFAULT, "negative shift integer\n");
		return 1;
	}

	n = mpl_nr_bits(bp);

	if (n > sizeof(int) * CHAR_BIT) {
		DEBUG(LOG_DEFAULT, "can't convert to int, overflow\n");
		return 1;
	}

	ret = mpl_to_uint(bp, &shift);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "can't convert to uint\n");
		return 1;
	}

	ret = mpl_copy(cp, ap);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "can't copy bnum\n");
		return 1;
	}

	ret = mpl_shr(cp, shift);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "can't convert to uint\n");
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);

	return 0;
}

/* String operations: */
int
varop_str_concat(struct variable*c, struct variable *a, struct variable *b)
{
	str_t *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);
	
	ap = var_cast_to_str(a);
	bp = var_cast_to_str(b);
	cp = var_str_ptr(c);

	str_concat(cp, ap, bp);

	var_force_type(c, VAR_STRING);

	return 0;
}

int
varop_str_sub(struct variable *res, struct variable *s, struct variable *start, struct variable *len)
{
	str_t *str;
	str_t *resstr;
	mpl_int *a, *b;
	long unsigned int first, n;
	int ret;

	str = var_cast_to_str(s);
	a = var_cast_to_bignum(start);
	b = var_cast_to_bignum(len);

	ret = mpl_to_uint(a, &first);
	if (ret != MPL_OK)
		goto err;
	ret = mpl_to_uint(b, &n);
	if (ret != MPL_OK)
		goto err;
	
	resstr = var_str_ptr(res);

	ret = str_substr(resstr, str, first, n);
	if (ret != 0)
		goto err;
	
	var_force_type(res, VAR_STRING);

	return 0;
err:
	return 1;
}

int
varop_str_len(struct variable *res, struct variable *a)
{
	str_t *str;
	mpl_int *bnum;
	int len;

	assert(res != NULL && a != NULL);

	str = var_cast_to_str(a);
	bnum = var_bignum_ptr(res);

	len = str_len(str);
	mpl_set_sint(bnum, len);

	var_force_type(res, VAR_BIGNUM);

	return 0;
}

/* Octstring operations: */
enum {
	PAD_LEFT,
	PAD_RIGHT,
};

static int
varop_padding(struct variable *dst, struct variable *src, struct variable *width, struct variable *filler, int type)
{
	octstr_t *dp, *sp, *fp;
	mpl_int *wp;
	long int sz;
	size_t flen;

	dp = var_octstr_ptr(dst);
	sp = var_cast_to_octstr(src);
	fp = var_cast_to_octstr(filler);
	wp = var_cast_to_bignum(width);

	//FIXME: write mpl_to_sint
	if (mpl_to_uint(wp, (unsigned long int *) &sz) != MPL_OK)
		return 1;
	
	flen = octstr_len(fp);
	if (flen == 0)
		return 0;
	/* padding size is width - size(src) */
	sz -= octstr_len(sp);

	octstr_reset(dp);

	if (type == PAD_RIGHT)
		octstr_append_octstr(dp, sp);
	
	while (sz > 0 && sz > flen) {
		octstr_append_octstr(dp, fp);
		sz -= flen;
	}

	if (sz > 0) {
		unsigned char *tmp;

		tmp = octstr_ptr(fp);
		octstr_append_n(dp, tmp, sz);
	}

	if (type == PAD_LEFT)
		octstr_append_octstr(dp, sp);

	var_force_type(dst, VAR_OCTSTRING);

	return 0;
}

int
varop_lpad(struct variable *dst, struct variable *src, struct variable *width, struct variable *filler)
{
	return varop_padding(dst, src, width, filler, PAD_LEFT);
}

int
varop_rpad(struct variable *dst, struct variable *src, struct variable *width, struct variable *filler)
{
	return varop_padding(dst, src, width, filler, PAD_RIGHT);
}

int
varop_oct_concat(struct variable *c, struct variable *a, struct variable *b)
{
	octstr_t *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);
	
	ap = var_cast_to_octstr(a);
	bp = var_cast_to_octstr(b);
	cp = var_octstr_ptr(c);

	octstr_concat(cp, ap, bp);

	var_force_type(c, VAR_OCTSTRING);

	return 0;
}

int
varop_octstr_sub(struct variable *res, struct variable *s, struct variable *start, struct variable *len)
{
	octstr_t *octstr;
	octstr_t *resstr;
	mpl_int *a, *b;
	long unsigned int first, n;
	int ret;

	octstr = var_cast_to_octstr(s);
	a = var_cast_to_bignum(start);
	b = var_cast_to_bignum(len);

	ret = mpl_to_uint(a, &first);
	if (ret != MPL_OK) {
		print_warn("too big\n");
		goto err;
	}
	ret = mpl_to_uint(b, &n);
	if (ret != MPL_OK) {
		print_warn("too big\n");
		goto err;
	}

	resstr = var_octstr_ptr(res);
	ret = octstr_substr(resstr, octstr, first, n);
	if (ret != 0) {
		print_warn("substr error\n");
		goto err;
	}

	var_force_type(res, VAR_OCTSTRING);

	return 0;
err:
	return 1;
}

int
varop_octstr_len(struct variable *res, struct variable *a)
{
	octstr_t *octstr;
	mpl_int *bnum;
	int len;

	assert(res != NULL && a != NULL);

	octstr = var_cast_to_octstr(a);
	bnum = var_bignum_ptr(res);

	len = octstr_len(octstr);
	mpl_set_sint(bnum, len);

	var_force_type(res, VAR_BIGNUM);

	return 0;
}

static int
varop_rand_wrapper(void *buf, size_t size, void *rndctx)
{
	return rand_get_rand_bytes((unsigned char *)buf, size);
}

/* Random */
int
varop_rand_int(struct variable *dst, struct variable *start, struct variable *stop)
{
	mpl_int *ap, *cp;
	struct variable tmp;
	int ret;
	int n;

	cp = var_bignum_ptr(dst);

	var_init(&tmp);

	if (varop_cmp(stop, start) != 1)
		goto err;

	if (varop_sub(&tmp, stop, start) != 0)
		goto err;
	
	ap = var_cast_to_bignum(&tmp);

	ret = mpl_rand_below(cp, ap, rand_get_rand_bytes, NULL);
	if (ret != MPL_OK)
		goto err;

	var_force_type(dst, VAR_BIGNUM);

	if (varop_add(dst, dst, start) != 0)
		goto err;

	return 0;
err:
	var_clear(&tmp);
	return 1;
}

#define BUF_SZ 1024
int
varop_rand_octs(struct variable *dst, struct variable *len)
{
	mpl_int *ap;
	octstr_t *octstr;
	unsigned char buf[BUF_SZ];
	int ret;
	unsigned long sz;

	assert(dst != NULL && len != NULL);

	ap = var_cast_to_bignum(len);
	octstr = var_octstr_ptr(dst);

	if (mpl_to_uint(ap, &sz) != MPL_OK)
		return 1;
	
	octstr_reset(octstr);

	do {
		int n;
		n = sz % BUF_SZ;
		ret = rand_get_rand_bytes(buf, n);
		if (ret == -1)
			return 1;

		octstr_append_n(octstr, buf, n);
		sz -= n;
	} while (sz > 0);

	var_force_type(dst, VAR_OCTSTRING);
	return 0;
}

/* Rel operations.
 * return:
 * 1 if a > b
 * -1 if b < a
 *  0 if a == b
 */
int
varop_cmp(struct variable *a, struct variable *b)
{
	mpl_int *ap, *bp;
	int res;
	
	assert(a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);

	res = mpl_cmp(ap, bp);
	
	switch (res) {
	case MPL_CMP_EQ:
		return 0;
	case MPL_CMP_GT:
		return 1;
	case MPL_CMP_LT:
		return -1;
	default:
		SHOULDNT_REACH();
	}
}

int
varop_is_true(struct variable *a)
{
	mpl_int *ap;

	assert(a != NULL);

	ap = var_cast_to_bignum(a);

	if (mpl_iszero(ap))
		return 0;
	else
		return 1;
}

