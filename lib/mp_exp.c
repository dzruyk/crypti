#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mp.h"
#include "mp_common.h"

extern void mp_canonicalize(mp_int *a);

int
mp_exp(mp_int *c, const mp_int *a, const mp_int *b)
{
	mp_int res, tmp, u, v;
	int rc;

	if ((rc = mp_initv(&res, &tmp, &u, &v, NULL)) != MP_OK)
		return rc;

	rc = mp_copy(&u, a);
	if (rc != MP_OK)
		goto err;

	rc = mp_copy(&v, b);
	if (rc != MP_OK)
		goto err;
	
	mp_set_one(&res);

	while(!mp_iszero(&v)) {
		if (mp_isodd(&v)) {
			mp_mul(&res, &res, &u);
			if (rc != MP_OK)
				goto err;
		}
		
		rc = mp_shr(&v, 1);
		if (rc != MP_OK)
			goto err;
		
		rc = mp_mul(&u, &u, &u);
		if (rc != MP_OK)
			goto err;
	}

	mp_copy(c, &res);
	mp_canonicalize(c);

	rc = MP_OK;
err:
	mp_clearv(&res, &tmp, &u, &v, NULL);
	return rc;
}

