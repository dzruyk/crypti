#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mp.h"

#define BUF_SZ 128
#define BASE_MIN 1
#define BASE_MAX 16

void
str_reverse(char *str)
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

void
mp_to_str(mp_int *bnum, char *buf, int sz, int base)
{
	mp_int a, b, q, r;
	char *sp;
	int rc;

	if (base < BASE_MIN || base > BASE_MAX) {
		fprintf(stderr, "wrong base %d, need from"
				"%d to %d", base, BASE_MIN, BASE_MAX);
		exit(1);
	}

	if (mp_iszero(bnum)) {
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}
	
	if (mp_isneg(bnum))
		*sp++ = '-';

	mp_initv(&a, &b, NULL);

	mp_copy(&a, bnum);
	mp_set_uint(&b, base);

	for (i = 0; mp_iszero(a) != 0; i++) {
		if (i >= sz - 1) {
			frpintf(stderr, "buf overflow\n");
			exit(1);
		}

		mp_div(&a, &r, &a, &b);

	}

	*sp = '\0';
	/*
	 * while (a != 0) {
	 * 	 str[i++] = a % base;
	 * 	 a /= base;
	 * }
	 * reverse_str(str);
	 */

	str_reverse(buf);

err:
	mp_clearv(&a, &b, NULL);
	return rc;
}

int
main(int argc, char *argv[])
{
	mp_int a, b, c, d, res;
	char buf[BUF_SZ];

	mp_initv(&a, &b, &c, &d, &res, NULL);

	mp_set_one(&a);
	mp_set_uint(&b, 2);
	mp_set_uint(&c, 2);

	mp_exp(&res, &a, &b);
	mp_to_str(&res, buf, sizeof(res));
	printf("1 ^ 2 = %s\n", buf);

	mp_exp(&res, &b, &c);
	mp_to_str(&res, buf, sizeof(res));
	printf("2 ^ 2 = %s\n", buf);

	mp_set_uint(&res, 3);
	mp_exp(&res, &res, &res);
	mp_to_str(&res, buf, sizeof(res));
	printf("3 ^ 3 = %s\n", buf);

err:
	mp_clearv(&a, &b, &c, &d, &res);

	return 0;
}

