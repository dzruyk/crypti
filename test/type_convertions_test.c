#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "macros.h"
#include "mp.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"


/*
 * NOTE:
 * assume that STR_BASE is 16
 */
int
test_bnum_str(struct variable *var, int num, char *expected)
{
	mp_int *ap;
	str_t *str;
	
	ap = var_bignum_ptr(var);

	mp_set_sint(ap, num);
	var_force_type(var, VAR_BIGNUM);
	str = var_cast_to_str(var);

	if (expected == NULL) {
		DEBUG(LOG_DEFAULT, "res = %s\n", str_ptr(str));
		return 0;
	} else if (strcmp(str_ptr(str), expected) != 0) {
		DEBUG(LOG_DEFAULT, "WARN: %s != %s\n", str_ptr(str), expected);
		return 1;
	}

	return 0;
}

void
bnum_to_str()
{
	struct variable a;
	int i, nerrors;
	struct {
		int num;
		char *expected;
	} vectors[] = {
		{0x1, "1"},
		{0x0, "0"},
		{0xAA, "AA"},
		{-0xAA, "-AA"},
		{-0, "0"},
		{0xABCDE, "ABCDE"},
	};

	var_init(&a);

	for (i = nerrors = 0; i< ARRSZ(vectors); i++) {
		int ret;
		ret = test_bnum_str(&a, vectors[i].num, vectors[i].expected);
		if (ret != 0)
			printf("error at %d: expected %s\n", i, vectors[i].expected);

		nerrors += ret;
	}
	
	if (nerrors == 0)
		printf("SUCCESS!\n");

	var_clear(&a);
}

void
bnum_to_octstr()
{
	struct variable a;

	var_init(&a);
	var_clear(&a);
}

void
bnum_to_bnum()
{

}

void
str_to_bnum()
{
	struct variable a;

	var_init(&a);

	var_clear(&a);
}

void
str_to_octstr()
{

}

void
str_to_str()
{

}

int
test_octstr_str(struct variable *var, char *buf, int sz, char *expected)
{
	octstr_t *octstr;
	str_t *str;

	octstr = var_octstr_ptr(var);

	octstr_reset(octstr);
	octstr_append_n(octstr, buf, sz);
	var_force_type(var, VAR_OCTSTRING);
	str = var_cast_to_str(var);

	if (expected == NULL) {
		DEBUG(LOG_DEFAULT, "res = %s\n", str_ptr(str));
		return 0;
	} else if (strcmp(str_ptr(str), expected) != 0) {
		DEBUG(LOG_DEFAULT, "WARN: %s != %s\n", str_ptr(str), expected);
		return 1;
	}

	return 0;
}

void
octstr_to_str()
{
	struct variable a;
	int i, nerrors;
	struct {
		char *buf;
		int sz;
		char *expected;
	} vectors[] = {
		{"\x00\x00\x00\x00\x00\x00", 6,
		    "\\x00\\x00\\x00\\x00\\x00\\x00"},
		{"", 0, ""},
		{"\xff", 1, "\\xFF"},
	};

	var_init(&a);

	for (i = nerrors = 0; i< ARRSZ(vectors); i++) {
		int ret;
		ret = test_octstr_str(&a, vectors[i].buf, vectors[i].sz,
		    vectors[i].expected);
		if (ret != 0)
			printf("error at %d: expected %s\n", i, vectors[i].expected);

		nerrors += ret;
	}

	if (nerrors == 0)
		printf("SUCCESS!\n");
	var_clear(&a);
}

void
octstr_to_bnum()
{

}

void
octstr_to_octstr()
{


}

int
main(int argc, char *argv[])
{
	printf("testing bnum_to_str:\n");
	bnum_to_str();
	printf("testing bnum_to_octstr:\n");
	bnum_to_octstr();
	printf("testing bnum_to_bnum:\n");
	bnum_to_bnum();
	
	printf("testing str_to_bnum:\n");
	str_to_bnum();
	printf("testing str_to_octstr:\n");
	str_to_octstr();
	printf("testing str_to_str\n");
	str_to_str();

	printf("testing octstring_to_string:\n");
	octstr_to_str();
	printf("testing octstring_to_bnum:\n");
	octstr_to_bnum();
	printf("testing octstring_to_octstring:\n");
	octstr_to_octstr();

	return 0;
}
