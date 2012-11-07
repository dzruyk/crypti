#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "mp.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"


/* bignum operations: */

typedef int (*bnum_op_func_t)(mp_int *c, const mp_int *a, const mp_int *b);

static inline int
varop_bnum_op(struct variable *c, struct variable *a, struct variable *b, bnum_op_func_t mp_func)
{
	assert(c != NULL && a != NULL && b != NULL && mp_func != NULL);
	int ret;

	mp_int *ap, *bp, *cp;

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
	assert(c != NULL && a != NULL && b != NULL);

	print_warn_and_die("WIP!\n");

	return 0;
}

int
varop_pow(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	print_warn_and_die("WIP!\n");
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
	assert(c != NULL && a != NULL && b != NULL);

	print_warn_and_die("WIP!\n");

}

int
varop_xor(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	print_warn_and_die("WIP!\n");

}

int
varop_and(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);

	print_warn_and_die("WIP!\n");

}

int
varop_cmp(struct variable *dst, struct variable *a, struct variable *b)
{
	assert(dst != NULL && a != NULL && b != NULL);

	mp_int *ap, *bp, *cp;
	int res;

	ap = var_cast_to_bignum(a);
	bp = var_cast_to_bignum(b);
	cp = var_bignum_ptr(dst);

	res = mp_cmp(ap, bp);
	if (res == MP_CMP_EQ) {
		mp_zero(cp);
	} else if (res == MP_CMP_GT) {
		mp_set_one(cp);
	} else if (res == MP_CMP_LT) {
		mp_set_one(cp);
		cp->sign = MP_SIGN_NEG;
	}
	return 0;
}

int
varop_not(struct variable *dst, struct variable *src)
{
	assert(dst != NULL && src != NULL);

	mp_int *ap, *bp;

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
	assert(c != NULL && a != NULL && b != NULL);
	
	print_warn_and_die("WIP!\n");

}

int
varop_shr(struct variable *c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);
	
	print_warn_and_die("WIP!\n");

}

/* String operations: */
int
varop_str_concat(struct variable*c, struct variable *a, struct variable *b)
{
	assert(c != NULL && a != NULL && b != NULL);
	
	str_t *ap, *bp, *cp;

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
	assert(c != NULL && a != NULL && b != NULL);
	
	octstr_t *ap, *bp, *cp;

	ap = var_cast_to_octstr(a);
	bp = var_cast_to_octstr(b);
	cp = var_octstr_ptr(c);

	octstr_concat(cp, ap, bp);

	var_force_type(c, VAR_OCTSTRING);

	return 0;
}

