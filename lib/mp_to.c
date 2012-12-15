#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mp.h"
#include "mp_common.h"

#define BASE_MIN 1
#define BASE_MAX 16

extern int mp_canonicalize(mp_int *a);

static void
strrev(char *str)
{
        char *end;
        char tmp;
        int sz; 

        sz = strlen(str);
        end = str + sz - 1;

        while (str < end) {
                tmp = *str;
                *str++ = *end;
                *end-- = tmp;
        }
}

int
mp_to_str(mp_int *bnum, char *buf, int sz, int base)
{
	mp_int a, b, r;
	char *sp;
	int rc;
	char ch;

	if (base < BASE_MIN || base > BASE_MAX) {
		return MP_ERR;
	}

	sp = buf;

	if (mp_iszero(bnum)) {
		if (sz < 2)
			return MP_ERR;
		buf[0] = '0';
		buf[1] = '\0';
		return MP_OK;
	}
	
	if (mp_isneg(bnum)) {
		*sp++ = '-';
		sz--;
	}

	mp_initv(&a, &b, &r, NULL);

	rc = mp_copy(&a, bnum);
	if (rc != MP_OK)
		goto err;

	mp_set_uint(&b, base);

	while (mp_iszero(&a) == 0) {
		if (sz <= 0) {
			rc = MP_ERR;
			goto err;
		}

		rc = mp_div(&a, &r, &a, &b);
		if (rc != MP_OK)
			goto err;
		
		ch = r.dig[0];
		if (ch < 10)
			*sp++ = ch + '0';
		else
			*sp++ = ch - 10 + 'A';
		sz--;
	}

	*sp = '\0';
	strrev(buf);

err:
	mp_clearv(&a, &b, &r, NULL);

	return rc;
}

int 
mp_to_uint(mp_int *a, unsigned long *val)
{
	int nbits;

	nbits = mp_nr_bits(a);

	if (nbits > sizeof(int) * CHAR_BIT)
		return MP_ERR;

	*val = a->dig[0];
	if (nbits > MP_INT_BITS) {
		*val <<= MP_INT_BITS;
		*val += a->dig[1];
	}

	return MP_OK;
}

