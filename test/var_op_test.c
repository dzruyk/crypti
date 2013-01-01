#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <mpl.h>
#include "str.h"
#include "octstr.h"
#include "variable.h"
#include "var_op.h"

char *
mpl_sprint(mpl_int *a, char *s, int slen)
{
	char *sp;
	int i;
	int res;
	
	sp = s;
	for (i = 0; i <= a->top; i++) {
		res = sprintf(s, "%d", a->dig[i]);
		if (slen < res + 1)
			return NULL;
		slen -= res;
		sp += res;
	}
	return s;
}

enum {
	TEST_LEN = 64,
};

int
main()
{
	struct variable a, b, c, res;
	mpl_int *ap, *bp, *cp, *resp;
	char bnum[TEST_LEN];

	var_initv(&a, &b, &c, &res, NULL);

	ap = var_bignum_ptr(&a);
	bp = var_bignum_ptr(&b);
	cp = var_bignum_ptr(&c);

	mpl_set_uint(ap, 42);
	mpl_set_uint(bp, 12);
	mpl_set_uint(cp, 6);

	printf("cp = 6 and mpl_sprintf shows %s\n", mpl_sprint(cp, bnum, TEST_LEN));

	var_force_type(&a, VAR_BIGNUM);
	var_force_type(&b, VAR_BIGNUM);
	var_force_type(&c, VAR_BIGNUM);

	varop_add(&res, &a, &b);

	resp = var_bignum_ptr(&res);

	printf("42 + 12 =  %s\n", mpl_sprint(resp, bnum, TEST_LEN));

	var_clearv(&a, &b, &c, &res, NULL);

	return 0;
}

