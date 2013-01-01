#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mpl.h>

#define BUF_SZ 256

void
exp_test()
{
	mpl_int a, b, c, d, res;
	char buf[BUF_SZ];

	mpl_initv(&a, &b, &c, &d, &res, NULL);

	mpl_set_one(&a);
	mpl_set_uint(&b, 2);
	mpl_set_uint(&c, 2);

	mpl_to_str(&a, buf, sizeof(buf), 10);
	printf("1 = %s\n", buf);

	mpl_exp(&res, &a, &b);
	mpl_to_str(&res, buf, sizeof(buf), 10);
	printf("1 ^ 2 = %s\n", buf);

	mpl_exp(&res, &b, &c);
	mpl_to_str(&res, buf, sizeof(buf), 10);
	printf("2 ^ 2 = %s\n", buf);

	mpl_set_uint(&res, 3);
	mpl_exp(&res, &res, &res);
	mpl_to_str(&res, buf, sizeof(buf), 10);
	printf("3 ^ 3 = %s\n", buf);

	mpl_set_uint(&res, 2);
	mpl_set_uint(&b, 428);
	mpl_exp(&res, &res, &b);
	mpl_to_str(&res, buf, sizeof(buf), 10);
	printf("2 ^ 1024 = 0x%s\n", buf);


	mpl_set_uint(&a, 0);
	printf("mpl_set_unt(&a, 0);\nmpl_iszero(a) == %s\n", mpl_iszero(&a) == 1 ? "TRUE" : "FALSE");

	mpl_clearv(&a, &b, &c, &d, &res, NULL);
}

int
main(int argc, char *argv[])
{
	exp_test();

	return 0;
}

