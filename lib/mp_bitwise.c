#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mp.h"
#include "mp_common.h"

extern void mp_canonicalize(mp_int *a);

/* Rewrite me later */
/* c = |a| & |b| */
int
mp_and(mp_int *c, const mp_int *a, const mp_int *b)
{
	_mp_int_t *ap, *bp, *cp;
	int i, rc;
	int btop, ctop;

	if (a->top < b->top) {
		const mp_int *tmp;
		tmp = a;
		a = b;
		b = tmp;
	}

	if ((rc = mp_ensure(c, a->top + 1)) != MP_OK)
		return rc;
	
	ap = a->dig;
	bp = b->dig;
	cp = c->dig;
	btop = b->top;
	ctop = c->top;
	
	for (i = 0; i <= btop; i++)
		*cp++ = (*ap++) & (*bp++);
	
	for (i = btop + 1; i < ctop; i++)
		*cp++ = 0;

	c->sign = MP_SIGN_POS;

	mp_canonicalize(c);

	return MP_OK;
}

/* Rewrite me later */
/* c = |a| | |b| */
int
mp_or(mp_int *c, const mp_int *a, const mp_int *b)
{
	_mp_int_t *ap, *bp, *cp;
	int i, rc;
	int atop, btop, ctop;

	if (a->top < b->top) {
		const mp_int *tmp;
		tmp = a;
		a = b;
		b = tmp;
	}

	if ((rc = mp_ensure(c, a->top + 1)) != MP_OK)
		return rc;
	
	ap = a->dig;
	bp = b->dig;
	cp = c->dig;
	atop = a->top;
	btop = b->top;
	ctop = c->top;

	for (i = 0; i <= btop; i++)
		*cp++ = (*ap++) | (*bp++);
	
	for (i = btop + 1; i <= atop; i++)
		*cp++ = *ap++;

	for (i = atop + 1; i < ctop; i++)
		*cp++ = 0;

	c->sign = MP_SIGN_POS;

	mp_canonicalize(c);
	
	return MP_OK;
}

/* Rewrite me later */
/* c = |a| ^ |b| */
int
mp_xor(mp_int *c, const mp_int *a, const mp_int *b)
{
	_mp_int_t *ap, *bp, *cp;
	int i, rc;
	int atop, btop, ctop;

	if (a->top < b->top) {
		const mp_int *tmp;
		tmp = a;
		a = b;
		b = tmp;
	}

	if ((rc = mp_ensure(c, a->top + 1)) != MP_OK)
		return rc;
	
	ap = a->dig;
	bp = b->dig;
	cp = c->dig;
	atop = a->top;
	btop = b->top;
	ctop = c->top;

	for (i = 0; i <= btop; i++)
		*cp++ = (*ap++) ^ (*bp++);
	
	for (i = btop + 1; i <= atop; i++)
		*cp++ = *ap++;

	for (i = atop + 1; i < ctop; i++)
		*cp++ = 0;

	c->sign = MP_SIGN_POS;

	mp_canonicalize(c);
	
	return MP_OK;
}

