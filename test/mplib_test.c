#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mp.h"

#define BUF_SZ 256
#define BASE_MIN 1
#define BASE_MAX 16

void
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
		fprintf(stderr, "wrong base %d, need from"
				"%d to %d", base, BASE_MIN, BASE_MAX);
		exit(1);
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

	mp_clearv(&a, &b, &r, NULL);

	return rc;
err:

	fprintf(stderr, "mp_to_str error");
	return rc;
}

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

	mp_clearv(&a, &b, &c, &d, &res, NULL);
}

int
main(int argc, char *argv[])
{
	exp_test();

	return 0;
}

