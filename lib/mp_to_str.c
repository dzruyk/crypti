#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include "mp.h"

int
mp_to_str(char *buf, int len, int base, const mp_int *a)
{
	mp_int x, y, r;
	int i, rc, nb, cmp;
	char *sp, *end;
	char c;

	if (base < 2 || base > 36)
		return MP_ERR;

	nb = 0;
	for (i = base; i > 0; i >>= 1)
		nb++;

	/* estimate the lenght of buf */
	if (len < mp_nr_bits(a)/nb + 3)
		return MP_ERR;

	if (mp_iszero(a)) {
		*buf++ =  '0';
		*buf   = '\0';
		return MP_OK;
	}

	if ((rc = mp_initv(&x, &y, &r, NULL)) != MP_OK)
		return rc;

	if ((rc = mp_copy(&y, a)) != MP_OK)
		goto err;

	mp_set_uint(&x, base);

	if (mp_isneg(&y)) {
		*buf++ = '-';
		y.sign = MP_SIGN_POS;
	}

	sp = buf;
	cmp = mp_cmp(&y, &x);
	while (cmp == MP_CMP_GT || cmp == MP_CMP_EQ) {
		rc = mp_div(&y, &r, &y, &x);
		if (rc != MP_OK)
			goto err;

		c = (char) r.dig[0];
		if (c >= 0 && c <= 9)
			*sp = c + '0';
		else
			*sp = c + 'a' - 10;

		sp++;
		cmp = mp_cmp(&y, &x);
	}

	c = (char) y.dig[0];
	if (c > 0) {
		if (c <= 9)
			*sp = c + '0';
		else
			*sp = c + 'a' - 10;
		sp++;
	}
	*sp-- = '\0';

	end = sp;
	sp = buf;

	while (sp < end) {
		char tmp;

		tmp    = *sp;
		*sp++  = *end;
		*end-- = tmp;
	}

	rc = MP_OK;
err:
	mp_clearv(&x, &y, &r, NULL);

	return rc;
}

