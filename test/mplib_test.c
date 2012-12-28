#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mp.h"

#define BUF_SZ 256

void
exp_test()
{
	mp_int a, b, c, d, res;
	char buf[BUF_SZ];

	mp_initv(&a, &b, &c, &d, &res, NULL);

	mp_set_one(&a);
	mp_set_uint(&b, 2);
	mp_set_uint(&c, 2);

	mp_to_str(&a, buf, sizeof(buf), 10);
	printf("1 = %s\n", buf);

	mp_exp(&res, &a, &b);
	mp_to_str(&res, buf, sizeof(buf), 10);
	printf("1 ^ 2 = %s\n", buf);

	mp_exp(&res, &b, &c);
	mp_to_str(&res, buf, sizeof(buf), 10);
	printf("2 ^ 2 = %s\n", buf);

	mp_set_uint(&res, 3);
	mp_exp(&res, &res, &res);
	mp_to_str(&res, buf, sizeof(buf), 10);
	printf("3 ^ 3 = %s\n", buf);

	mp_set_uint(&res, 2);
	mp_set_uint(&b, 428);
	mp_exp(&res, &res, &b);
	mp_to_str(&res, buf, sizeof(buf), 10);
	printf("2 ^ 1024 = 0x%s\n", buf);


	mp_set_uint(&a, 0);
	printf("mp_set_unt(&a, 0);\nmp_iszero(a) == %s\n", mp_iszero(&a) == 1 ? "TRUE" : "FALSE");

	mp_clearv(&a, &b, &c, &d, &res, NULL);
}

int
main(int argc, char *argv[])
{
	exp_test();

	return 0;
}

