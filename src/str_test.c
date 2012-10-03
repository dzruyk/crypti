#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "str.h"

#define A_STR "HELLO, "
#define B_STR "WORLD!"

int
main(int argc, char *argv[])
{
	str_t *a, *b, *c;

	a = str_new(A_STR, strlen(A_STR));
	b = str_new(B_STR, strlen(B_STR));

	printf("str_len(a) = %d\n", str_len(a));
	printf("str_len(b) = %d\n", str_len(b));

	c = str_concat(a, b);

	printf("str_len(c) = %d\n", str_len(c));

	str_print(a);
	printf("\n");
	str_print(b);
	printf("\n");
	str_print(c);
	printf("\n");

	str_del(a);
	str_del(b);

	a = str_dup(c);
	b = str_sub(c, 2, 27);

	printf("dup result:\n");
	str_print(a);
	printf("\n");

	printf("sub result:\n");
	str_print(b);
	printf("\n");


	str_del(a);
	str_del(b);
	str_del(c);

	return 0;
}

