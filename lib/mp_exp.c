#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mp.h"
#include "mp_common.h"

extern void mp_canonicalize(mp_int *a);

int
mp_exp(mp_int *c, const mp_int *a, const mp_int *b)
{


	mp_canonicalize(c);

	return MP_OK;
}

