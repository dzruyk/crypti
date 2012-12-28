#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "common.h"
#include "log.h"
#include "macros.h"
#include "mp.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"


/* bignum operations: */

typedef int (*bnum_op_func_t)(mp_int *c, const mp_int *a, const mp_int *b);

static inline int
varop_bnum_op(struct variable *c, struct variable *a, struct variable *b, bnum_op_func_t mp_func)
{
	int ret;
	mp_int *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL && mp_func != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	ret = mp_func(cp, ap, bp);
	if (ret != MP_OK) {
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);

	return 0;
}

/* Number operations: */
int
varop_add(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	if (varop_bnum_op(c, a, b, mp_add) != 0) {
		return 1;
	}

	return 0;
}

int
varop_sub(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	if (varop_bnum_op(c, a, b, mp_sub) != 0) {
		return 1;
	}

	return 0;
}

int
varop_mul(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	if (varop_bnum_op(c, a, b, mp_mul) != 0) {
		return 1;
	}

	return 0;
}

int
varop_div(struct variable *c, struct variable *a, struct variable *b)
{
	mp_int *ap, *bp, *cp;
	int ret;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	ret = mp_div(cp, NULL, ap, bp);
	if (ret != MP_OK) {
		return 1;
	}

	var_force_type(c, VAR_BIGNUM);

	return 0;
}

int
varop_pow(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	if (varop_bnum_op(c, a, b, mp_exp) != 0) {
		return 1;
	}
	
	return 0;
}

int
varop_gcd(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	if (varop_bnum_op(c, a, b, mp_gcd) != 0) {
		return 1;
	}

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
varop_not(struct variable *dst, struct variable *src)
{
	mp_int *ap, *bp;

	assert(dst != NULL && src != NULL);

	ap = var_cast_to_bignum(src);
	bp = var_bignum_ptr(dst);

	if (mp_iszero(ap))
		mp_set_one(bp);
	else
		mp_zero(bp);
	
	return 0;
}

int
varop_neg(struct variable *res, struct variable *var)
{
	assert(res != NULL && var != NULL);

	mp_int *dst, *src;

	src = var_cast_to_bignum(var);
	dst = var_bignum_ptr(res);
	
	mp_copy(dst, src);

	dst->sign = !src->sign;

	return 0;
}

int
varop_shl(struct variable *c, struct variable *a, struct variable *b)
{
	int n, ret;
	unsigned long shift;
	mp_int *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	if (mp_isneg(bp)) {
		DEBUG(LOG_DEFAULT, "negative shift integer\n");
		return 1;
	}

	n = mp_nr_bits(bp);

	if (n > sizeof(int) * CHAR_BIT) {
		DEBUG(LOG_DEFAULT, "can't convert to int, overflow\n");
		return 1;
	}

	ret = mp_to_uint(bp, &shift);
	if (ret != MP_OK) {
		DEBUG(LOG_DEFAULT, "can't convert to uint\n");
		return 1;
	}
	
	ret = mp_copy(cp, ap);
	if (ret != MP_OK) {
		DEBUG(LOG_DEFAULT, "can't copy bnum\n");
		return 1;
	}

	ret = mp_shl(cp, shift);
	if (ret != MP_OK) {
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
	mp_int *ap, *bp, *cp;

	assert(c != NULL && a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(c);

	if (mp_isneg(bp)) {
		DEBUG(LOG_DEFAULT, "negative shift integer\n");
		return 1;
	}

	n = mp_nr_bits(bp);

	if (n > sizeof(int) * CHAR_BIT) {
		DEBUG(LOG_DEFAULT, "can't convert to int, overflow\n");
		return 1;
	}

	ret = mp_to_uint(bp, &shift);
	if (ret != MP_OK) {
		DEBUG(LOG_DEFAULT, "can't convert to uint\n");
		return 1;
	}

	ret = mp_copy(cp, ap);
	if (ret != MP_OK) {
		DEBUG(LOG_DEFAULT, "can't copy bnum\n");
		return 1;
	}

	ret = mp_shr(cp, shift);
	if (ret != MP_OK) {
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

/* Octstring operations: */
int
varop_oct_concat(struct variable*c, struct variable *a, struct variable *b)
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

/* Rel operations.
 * return:
 * 1 if a > b
 * -1 if b < a
 *  0 if a == b
 */
int
varop_cmp(struct variable *a, struct variable *b)
{
	mp_int *ap, *bp;
	int res;
	
	assert(a != NULL && b != NULL);

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);

	res = mp_cmp(ap, bp);
	
	switch (res) {
	case MP_CMP_EQ:
		return 0;
	case MP_CMP_GT:
		return 1;
	case MP_CMP_LT:
		return -1;
	default:
		SHOULDNT_REACH();
	}
}

int
varop_is_true(struct variable *a)
{
	mp_int *ap;

	assert(a != NULL);

	ap = var_cast_to_bignum(a);

	if (mp_iszero(ap))
		return 0;
	else
		return 1;
}

