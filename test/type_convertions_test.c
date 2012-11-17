#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mp.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"


void
test_bnum_str(struct variable *var, int num, char *expected)
{
	mp_int *ap;
	str_t *str;
	
	ap = var_bignum_ptr(var);

	mp_set_sint(ap, num);
	var_force_type(var, VAR_BIGNUM);
	str = var_cast_to_str(var);

	if (expected == NULL)
		printf("res = %s\n", str_ptr(str));
	else if (strcmp(str_ptr(str), expected) != 0) {
		printf("WARN: %s != %s\n", str_ptr(str), expected);
	} else {
	       printf("SUCCESS!\n");
	}
}

void
bnum_to_str()
{
	struct variable a;

	var_init(&a);

	test_bnum_str(&a, 0x1, "1");
	test_bnum_str(&a, 0x0, "0");
	test_bnum_str(&a, 0xAA, "AA");
	test_bnum_str(&a, -0xAA, "-AA");
	test_bnum_str(&a, -0, "0");
	test_bnum_str(&a, 0xABCDE, "ABCDE");

	var_clear(&a);
}

void
str_to_bnum()
{
	struct variable a;

	var_init(&a);

	var_clear(&a);
}

int
main()
{
	printf("testing bnum_to_str:\n");
	bnum_to_str();
	
	printf("testing str_to_bnum:\n");
	str_to_bnum();

	return 0;
}
