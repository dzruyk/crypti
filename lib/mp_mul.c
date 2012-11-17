#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "mp.h"
#include "mp_common.h"

extern void mp_canonicalize(mp_int *a);

int
_mp_mul_toom_three(mp_int *c, const mp_int *a, const mp_int *b)
{
	mp_int p, p0, p1, p2, p3, p4;
	mp_int q, q0, q1, q2, q3, q4;
	mp_int m0, m1, m2;
	mp_int n0, n1, n2;
	mp_int three;
	_mp_int_t num;
	int n, top, atop, btop, nbits;
	int rc;

	rc = mp_initv(&p, &p0 ,&p1, &p2, &p3, &p4, &q, &q0, &q1, &q2, &q3, &q4, NULL);
	if (rc != MP_OK)
		return rc;

	num = 0;

	atop = a->top;
	btop = b->top;

	n = (atop > btop) ? atop : btop;
	n++;
	n = (n % 3 == 0) ? n/3 : n/3 + 1;
	top = n - 1;

	/* set up initial values */
	m0.sign  = MP_SIGN_POS;
	m0.alloc = 0;
	m0.top   = (atop < top) ? atop : top;
	m0.dig   = a->dig;

	m1.sign  = MP_SIGN_POS;
	m1.alloc = 0;
	if (atop < top) {
		m1.top = -1;
		m1.dig = &num;
	} else {
		m1.top = (atop < 2*top) ? atop - top : top;
		m1.dig = a->dig + n;
	}

	m2.sign  = MP_SIGN_POS;
	m2.alloc = 0;
	if (atop < 2*top) {
		m2.top = -1;
		m2.dig = &num;
	} else {
		m2.top = (atop < 3*top) ? atop - (2*top) : top;
		m2.dig = a->dig + (2*n);
	}

	n0.sign  = MP_SIGN_POS;
	n0.alloc = 0;
	n0.top   = (btop < top) ? btop : top;
	n0.dig   = b->dig;

	n1.sign  = MP_SIGN_POS;
	n1.alloc = 0;
	if (btop < top) {
		n1.top = -1;
		n1.dig = &num;
	} else {
		n1.top = (btop < 2*top) ? btop - top : top;
		n1.dig = b->dig + n;
	}

	n2.sign  = MP_SIGN_POS;
	n2.alloc = 0;
	if (btop < 2*top) {
		n2.top = -1;
		n2.dig = &num;
	} else {
		n2.top = (btop < 3*top) ? btop - (2*top) : top;
		n2.dig = b->dig + (2*n);
	}

	/* prepare polynomial coefficients for p */
	/* p = m0 + m2 */
	rc = mp_add(&p, &m0, &m2);
	if (rc != MP_OK)
		goto err;

	/* p0 = m0 */
	rc = mp_copy(&p0, &m0);
	if (rc != MP_OK)
		goto err;

	/* p1 = p + m1 */
	rc = mp_add(&p1, &p, &m1);
	if (rc != MP_OK)
		goto err;

	/* p2 = p - m1 */
	rc = mp_sub(&p2, &p, &m1);
	if (rc != MP_OK)
		goto err;

	/* p3 = (p2 + m2)*2 - m0 */
	rc = mp_add(&p3, &p2, &m2);
	if (rc != MP_OK)
		goto err;

	rc = mp_shl(&p3, 1);
	if (rc != MP_OK)
		goto err;

	rc = mp_sub(&p3, &p3, &m0);
	if (rc != MP_OK)
		goto err;

	/* p4 = m2 */
	rc = mp_copy(&p4, &m2);
	if (rc != MP_OK)
		goto err;

	/* prepare polynomial coefficients for q */
	/* q = n0 + n2 */
	rc = mp_add(&q, &n0, &n2);
	if (rc != MP_OK)
		goto err;

	/* q0 = n0 */
	rc = mp_copy(&q0, &n0);
	if (rc != MP_OK)
		goto err;

	/* q1 = q + n1 */
	rc = mp_add(&q1, &q, &n1);
	if (rc != MP_OK)
		goto err;

	/* q2 = q - n1 */
	rc = mp_sub(&q2, &q, &n1);
	if (rc != MP_OK)
		goto err;

	/* q3 = (q2 + n2)*2 - n0 */
	rc = mp_add(&q3, &q2, &n2);
	if (rc != MP_OK)
		goto err;

	rc = mp_shl(&q3, 1);
	if (rc != MP_OK)
		goto err;

	rc = mp_sub(&q3, &q3, &n0);
	if (rc != MP_OK)
		goto err;

	/* q4 = n2 */
	rc = mp_copy(&q4, &n2);
	if (rc != MP_OK)
		goto err;

	/* pointwise multiplication */
	/* q0 = q0 * p0 */
	rc = mp_mul(&q0, &q0, &p0);
	if (rc != MP_OK)
		goto err;

	/* q1 = q1 * p1 */
	rc = mp_mul(&q1, &q1, &p1);
	if (rc != MP_OK)
		goto err;

	/* q2 = q2 * p2 */
	rc = mp_mul(&q2, &q2, &p2);
	if (rc != MP_OK)
		goto err;

	/* q3 = q3 * p3 */
	rc = mp_mul(&q3, &q3, &p3);
	if (rc != MP_OK)
		goto err;

	/* q4 = q4 * p4 */
	rc = mp_mul(&q4, &q4, &p4);
	if (rc != MP_OK)
		goto err;

	/* interpolation */
	/* p0 = q0 */
	rc = mp_copy(&p0, &q0);
	if (rc != MP_OK)
		goto err;

	/* p4 = q4 */
	rc = mp_copy(&p4, &q4);
	if (rc != MP_OK)
		goto err;

	/* p3 = (q3 - q1)/3 */
	rc = mp_sub(&p3, &q3, &q1);
	if (rc != MP_OK)
		goto err;

	num = 3;

	three.sign  = MP_SIGN_POS;
	three.alloc = 0;
	three.top   = 0;
	three.dig   = &num;

	rc = mp_div(&p3, NULL, &p3, &three);
	if (rc != MP_OK)
		goto err;

	/* p1 = (q1 - q2)/2 */
	rc = mp_sub(&p1, &q1, &q2);
	if (rc != MP_OK)
		goto err;

	rc = mp_shr(&p1, 1);
	if (rc != MP_OK)
		goto err;

	/* p2 = q2 - q0 */
	rc = mp_sub(&p2, &q2, &q0);
	if (rc != MP_OK)
		goto err;

	/* p3 = (p2 - p3)/2 + 2*q4 */
	rc = mp_sub(&p3, &p2, &p3);
	if (rc != MP_OK)
		goto err;

	rc = mp_shr(&p3, 1);
	if (rc != MP_OK)
		goto err;

	rc = mp_shl(&q4, 1);
	if (rc != MP_OK)
		goto err;

	rc = mp_add(&p3, &p3, &q4);
	if (rc != MP_OK)
		goto err;

	/* p2 = p2 + p1 - p4 */
	rc = mp_add(&p2, &p2, &p1);
	if (rc != MP_OK)
		goto err;

	rc = mp_sub(&p2, &p2, &p4);
	if (rc != MP_OK)
		goto err;

	/* p1 = p1 - p3 */
	rc = mp_sub(&p1, &p1, &p3);
	if (rc != MP_OK)
		goto err;

	/* evaluate the result */
	mp_zero(c);

	rc = mp_copy(c, &p0);
	if (rc != MP_OK)
		goto err;

	nbits = MP_INT_BITS * n;

	rc = mp_shl(&p1, nbits);
	if (rc != MP_OK)
		goto err;

	rc = mp_add(c, c, &p1);
	if (rc != MP_OK)
		goto err;

	rc = mp_shl(&p2, 2*nbits);
	if (rc != MP_OK)
		goto err;

	rc = mp_add(c, c, &p2);
	if (rc != MP_OK)
		goto err;

	rc = mp_shl(&p3, 3*nbits);
	if (rc != MP_OK)
		goto err;

	rc = mp_add(c, c, &p3);
	if (rc != MP_OK)
		goto err;

	rc = mp_shl(&p4, 4*nbits);
	if (rc != MP_OK)
		goto err;

	rc = mp_add(c, c, &p4);
	if (rc != MP_OK)
		goto err;

	c->sign = (a->sign == b->sign) ? MP_SIGN_POS : MP_SIGN_NEG;

	rc = MP_OK;
err:
	mp_clearv(&p, &p0 ,&p1, &p2, &p3, &p4, &q, &q0, &q1, &q2, &q3, &q4, NULL);

	return rc;
}

int
_mp_mul_karatsuba(mp_int *c, const mp_int *a, const mp_int *b)
{
	mp_int z0, z2;
	mp_int tmp1, tmp2;
	mp_int x0, x1, y0, y1;
	unsigned int n;
	int atop, btop, rc;

	atop = a->top;
	btop = b->top;

	rc = mp_initv(&z0, &z2, &tmp1, &tmp2, NULL);

	if (rc != MP_OK)
		return rc;

	/* Select half digits of the smallest number. */
	n = ((atop > btop) ? (btop+1) : (atop+1)) >> 1;

	/* Set up intermediate values used to calculate partial multiplications. */
	x0.dig = a->dig;
	x0.sign = MP_SIGN_POS;
	x0.top = n-1;
	x0.alloc = 0;

	x1.dig = a->dig + n;
	x1.sign = MP_SIGN_POS;
	x1.top = atop - n;
	x1.alloc = 0;

	y0.dig = b->dig;
	y0.sign = MP_SIGN_POS;
	y0.top = n-1;
	y0.alloc = 0;

	y1.dig = b->dig + n;
	y1.sign = MP_SIGN_POS;
	y1.top = btop - n;
	y1.alloc = 0;

	/* calculate z2 = x1 * y1 */
	if ((rc = mp_mul(&z2, &x1, &y1)) != MP_OK)
		goto err;

	/* calculate z0 = x0 * y0 */
	if ((rc = mp_mul(&z0, &x0, &y0)) != MP_OK)
		goto err;

	/* calculate tmp1 = x1 + x0 and tmp2 = y1 + y0 */
	if ((rc = mp_add(&tmp1, &x1, &x0)) != MP_OK)
		goto err;

	if ((rc = mp_add(&tmp2, &y1, &y0)) != MP_OK)
		goto err;

	/* calculate z1 = (x1 + x0)*(y1 + y0) - z2 - z0, store result in tmp1 */
	if ((rc = mp_mul(&tmp1, &tmp1, &tmp2)) != MP_OK)
		goto err;
	if ((rc = mp_sub(&tmp1, &tmp1, &z2)) != MP_OK)
		goto err;
	if ((rc = mp_sub(&tmp1, &tmp1, &z0)) != MP_OK)
		goto err;

	/* exponentiate z2 to 2^2n and z1 (tmp1) to 2^n */
	if ((rc = mp_shl(&z2, (n*MP_INT_BITS) << 1)) != MP_OK)
		goto err;
	if ((rc = mp_shl(&tmp1, n*MP_INT_BITS)) != MP_OK)
		goto err;

	/* c = z0 + z1 + z2 */
	if ((rc = mp_add(c, &z0, &tmp1)) != MP_OK)
		goto err;

	if ((rc = mp_add(c, c, &z2)) != MP_OK)
		goto err;

	rc = MP_OK;
err:
	mp_clearv(&z0, &z2, &tmp1, &tmp2, NULL);
	
	return rc;
}

/* Multiplies c = |a| * |b|, |a| > |b| using Comba O(n^2) method. */
int
_mp_mul_comba(mp_int *c, const mp_int *a, const mp_int *b)
{
	_mp_int_t tmp[MP_COMBA_STACK];
	_mp_int_t *ap, *bp, *cp;
	_mp_long_t r;
	int i, ai, rc, bplus1;
	unsigned int amin, bmin, amax;
	int atop, btop, maxtop, oldtop;

	atop = a->top;
	btop = b->top;
	bplus1 = btop + 1;
	maxtop = atop + btop + 1;
	oldtop = c->top;

	if ((rc = mp_ensure(c, maxtop+1)) != MP_OK)
		return rc;

	c->top = maxtop;
	r = 0;

	for (i = 0, cp = tmp; i < maxtop; i++, cp++) {

		amin = (i >= bplus1) ? i-btop : 0;
		bmin = (i >= bplus1) ? btop : i;
		amax = (i > atop) ? atop : i;

		ap = a->dig + amin;
		bp = b->dig + bmin;

		*cp = 0;

		for (ai = amin; ai <= amax; ai++)
			r += (_mp_long_t)(*ap++) * (_mp_long_t)(*bp--);

		*cp = r & MP_INT_MASK;
		r >>= MP_INT_BITS;
	}

	*cp = r;

	ap = tmp;
	bp = c->dig;

	for (i = 0; i <= maxtop; i++)
		*bp++ = *ap++;

	for (; i <= oldtop; i++)
		*bp++ = 0;

	mp_canonicalize(c);

	return MP_OK;
}

/* 
 * Slightly modified Comba multiplication calculates upper part
 * of the result starting from digit.
 */
int
_mp_mul_comba_upper(mp_int *c, const mp_int *a, const mp_int *b, int digit)
{
	_mp_int_t tmp[MP_COMBA_STACK];
	_mp_int_t *ap, *bp, *cp;
	_mp_long_t r;
	int i, ai, rc, bplus1;
	unsigned int amin, bmin, amax;
	int atop, btop, maxtop, oldtop;

	atop = a->top;
	btop = b->top;
	bplus1 = btop + 1;
	maxtop = atop + btop + 1;
	oldtop = c->top;

	if ((rc = mp_ensure(c, maxtop+1)) != MP_OK)
		return rc;

	c->top = maxtop;
	r = 0;

	for (i = digit, cp = tmp + digit; i < maxtop; i++, cp++) {

		amin = (i >= bplus1) ? i-btop : 0;
		bmin = (i >= bplus1) ? btop : i;
		amax = (i > atop) ? atop : i;

		ap = a->dig + amin;
		bp = b->dig + bmin;

		*cp = 0;

		for (ai = amin; ai <= amax; ai++)
			r += (_mp_long_t)(*ap++) * (_mp_long_t)(*bp--);

		*cp = r & MP_INT_MASK;
		r >>= MP_INT_BITS;
	}

	*cp = r;

	ap = tmp + digit;
	bp = c->dig;

	/* Zero lower digits of the result. */
	for (i = 0; i < digit; i++)
		*bp++ = 0;

	for (; i <= maxtop; i++)
		*bp++ = *ap++;
	
	/* Zero unused upper digits of the result. */
	for (; i <= oldtop; i++)
		*bp++ = 0;

	mp_canonicalize(c);

	return MP_OK;
}

int
_mp_mul_comba_ndig(mp_int *c, const mp_int *a, const mp_int *b, int ndig)
{
	_mp_int_t tmp[MP_COMBA_STACK];
	_mp_int_t *ap, *bp, *cp;
	_mp_long_t r;
	int i, ai, rc, bplus1;
	unsigned int amin, bmin, amax;
	int atop, btop, oldtop;
	int n, nmax;

	atop = a->top;
	btop = b->top;
	oldtop = c->top;
	bplus1 = btop + 1;

	nmax = atop + btop + 2;
	ndig = MIN(ndig, nmax);
	n = (ndig == nmax) ? ndig-1 : ndig;

	if ((rc = mp_ensure(c, n)) != MP_OK)
		return rc;
	r = 0;

	for (i = 0, cp = tmp; i < n; i++, cp++) {

		amin = (i >= bplus1) ? i-btop : 0;
		bmin = (i >= bplus1) ? btop : i;
		amax = (i > atop) ? atop : i;

		ap = a->dig + amin;
		bp = b->dig + bmin;

		*cp = 0;

		for (ai = amin; ai <= amax; ai++)
			r += (_mp_long_t)(*ap++) * (_mp_long_t)(*bp--);

		*cp = r & MP_INT_MASK;
		r >>= MP_INT_BITS;
	}

	*cp = r;

	ap = tmp;
	bp = c->dig;
	c->top = ndig - 1;

	for (i = 0; i < ndig; i++)
		*bp++ = *ap++;

	for (; i <= oldtop; i++)
		*bp++ = 0;

	mp_canonicalize(c);

	return MP_OK;
}

/* Multiplies c = |a| * |b|, |a| > |b| using school O(n^2) multiplication. */
int
_mp_mul_school(mp_int *c, const mp_int *a, const mp_int *b)
{
	mp_int tmp;
	_mp_int_t *ap, *bp, *cp;
	_mp_long_t r;
	int i, j, rc;
	int atop, btop;
	int maxtop;

	maxtop = a->top + b->top + 1;

	if ((rc = mp_init(&tmp)) != MP_OK)
		return rc;

	if ((rc = mp_ensure(&tmp, maxtop+1)) != MP_OK)
		return rc;

	atop = a->top;
	btop = b->top;
	tmp.top = maxtop;

	for (bp = b->dig, i = 0; i <= btop; ++bp, i++) {

		/* Reset carry and initialize pointers to c[i] and a. */
		r  = 0;
		cp = tmp.dig + i;
		ap = a->dig;

		/* Multiply-add with carry loop. Using index notation the following
		 * code implements c[i+j] += a[j]*b[i]. */
		for (j = 0; j <= atop; j++) {
			r += ((_mp_long_t)*bp) * ((_mp_long_t)(*ap++)) + ((_mp_long_t)*cp);
			*cp++ = r & (_mp_long_t)MP_INT_MASK;
			r >>= (_mp_long_t)MP_INT_BITS;
		}

		*cp = r;
	}

	if ((rc = mp_copy(c, &tmp)) != MP_OK) {
		mp_clear(&tmp);
		return rc;
	}

	mp_clear(&tmp);
	mp_canonicalize(c);

	return MP_OK;
}

int
mp_mul_dig(mp_int *c, const mp_int *a, _mp_int_t b)
{
	int i, n, rc, sign;
	_mp_long_t r;
	_mp_int_t *ap, *cp;

	if (mp_iszero(a) || b == 0) {
		mp_zero(c);
		return MP_OK;
	}

	if ((rc = mp_ensure(c, a->top + 2)) != MP_OK)
		return rc;

	n = a->top;
	ap = a->dig;
	sign = a->sign;

	/* Zero unused digits in result. */
	cp = c->dig + c->top;
	for (i = c->top; i > n; i--)
		*cp-- = 0;

	r = 0;
	cp = c->dig;

	for (i = 0; i <= n; i++) {
		r += (_mp_long_t)b * (_mp_long_t)(*ap++);
		*cp++ = r & (_mp_long_t)MP_INT_MASK;
		r >>= MP_INT_BITS;
	}

	*cp = r;

	c->top = n + 1;
	c->sign = sign;

	mp_canonicalize(c);

	return MP_OK;
}

int
mp_mul(mp_int *c, const mp_int *a, const mp_int *b)
{
	int rc, sign;
	const mp_int *u, *v;

	if (mp_iszero(a) || mp_iszero(b)) {
		mp_zero(c);
		return MP_OK;
	}

	sign = (a->sign != b->sign) ? MP_SIGN_NEG : MP_SIGN_POS;

	if (a->top > b->top) {
		u = a;
		v = b;
	} else {
		u = b;
		v = a;
	}

	if (MIN(u->top+1, v->top+1) >= MP_KARATSUBA_CUTOFF)
		rc = _mp_mul_karatsuba(c, u, v);
	else if (u->top + v->top + 2 <= MP_COMBA_STACK &&
	    v->top+1 <= MP_COMBA_DEPTH)
		rc = _mp_mul_comba(c, u, v);
	else
		rc = _mp_mul_school(c, u, v);

	if (rc != MP_OK)
		return rc;

	c->sign = sign;
	return MP_OK;
}

int
mp_mul_ndig(mp_int *c, const mp_int *a, const mp_int *b, int ndig)
{
	mp_int tmp;
	_mp_int_t *ap, *bp, *cp;
	_mp_long_t r;
	int i, j, n, m, rc;
	int nmax, sign;

	if (mp_iszero(a) || mp_iszero(b)) {
		mp_zero(c);
		return MP_OK;
	}

	if (a->sign != b->sign)
		sign = MP_SIGN_NEG;
	else
		sign = MP_SIGN_POS;

	/* Clamp maximum number of result digits produced. */
	nmax = a->top + b->top + 2;
	ndig = MIN(nmax, ndig);

	if ((ndig <= MP_COMBA_DEPTH || MIN(a->top+1, b->top+1) <= MP_COMBA_DEPTH) &&
	    ndig <= MP_COMBA_STACK) {
		if ((rc = _mp_mul_comba_ndig(c, a, b, ndig)) != MP_OK)
			return rc;
		c->sign = sign;
		return MP_OK;
	}

	if ((rc = mp_init(&tmp)) != MP_OK)
		return rc;

	/* Ensure place for final carry digit. */
	if ((rc = mp_ensure(&tmp, (ndig > nmax) ? ndig+1 : ndig)) != MP_OK)
		return rc;

	n = MIN(ndig, b->top+1);

	for (bp = b->dig, i = 0; i < n; ++bp, i++) {

		/* Reset carry and initialize pointers to c[i] and a. */
		r  = 0;
		cp = tmp.dig + i;
		ap = a->dig;

		/* Each row is less by i if `a' has more digits than required. */
		m = MIN(ndig - i, a->top+1);

		/* Multiply-add with carry loop. Using index notation the following
		 * code implements c[i+j] += a[j]*b[i]. */
		for (j = 0; j < m; j++) {
			r += ((_mp_long_t)*bp) * ((_mp_long_t)(*ap++)) + ((_mp_long_t)*cp);
			*cp++ = r & (_mp_long_t)MP_INT_MASK;
			r >>= (_mp_long_t)MP_INT_BITS;
		}

		/* 
		 * We always have place for the final carry digit and checking
		 * that i+j < ndig is worthless here.
		 */
		*cp = r;
	}

	tmp.top = ndig - 1;

	if ((rc = mp_copy(c, &tmp)) != MP_OK) {
		mp_clear(&tmp);
		return rc;
	}

	c->sign = sign;

	mp_clear(&tmp);
	mp_canonicalize(c);

	return MP_OK;
}

