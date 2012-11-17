#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "crypti.h"
#include "macros.h"
#include "mp.h"
#include "str.h"
#include "octstr.h"
#include "variable.h"
#include "type_convertions.h"

/* FIXME: Need to set as global variable. */
#define STR_BASE 16

static void *string_to_bignum(struct variable *to, const struct variable *from);
static void *string_to_octstring(struct variable *to, const struct variable *from);
static void *string_to_string(struct variable *to, const struct variable *from);

static void *octstring_to_bignum(struct variable *to, const struct variable *from);
static void *octstring_to_octstring(struct variable *to, const struct variable *from);
static void *octstring_to_string(struct variable *to, const struct variable *from);

static void *bignum_to_bignum(struct variable *to, const struct variable *from);
static void *bignum_to_octstring(struct variable *to, const struct variable *from);
static void *bignum_to_string(struct variable *to, const struct variable *from);

static int type_conv_cmp(const void *a, const void *b);

typedef void *(*type_converter_t)(struct variable *to, const struct variable *from);

struct type_conv {
	int from_type;
	int to_type;
	type_converter_t func;
};

struct type_conv func_table[] = {
	{ VAR_BIGNUM, VAR_BIGNUM, bignum_to_bignum},
	{ VAR_BIGNUM, VAR_OCTSTRING, bignum_to_octstring},
	{ VAR_BIGNUM, VAR_STRING, bignum_to_string},
	
	{ VAR_OCTSTRING, VAR_BIGNUM, octstring_to_bignum},
	{ VAR_OCTSTRING, VAR_OCTSTRING, octstring_to_octstring},
	{ VAR_OCTSTRING, VAR_STRING, octstring_to_string},

	{ VAR_STRING, VAR_BIGNUM, string_to_bignum },
	{ VAR_STRING, VAR_OCTSTRING, string_to_octstring },
	{ VAR_STRING, VAR_STRING, string_to_string },
};

void *convert_value(struct variable *dst_var, int to_type, struct variable *src_var, int from_type)
{
	struct type_conv conv, *res;

	conv.from_type = from_type;
	conv.to_type = to_type;

	res = bsearch(&conv, func_table, ARRSZ(func_table), sizeof(func_table[0]), type_conv_cmp);

	if (res == NULL)
		print_warn_and_die("can't force type, programmer mistake\n");

	return res->func(dst_var, src_var);
}

static void *
string_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to bignum\n");
	mp_int *bnum;
	str_t *str;

	str = var_str_ptr((struct variable *)from);
	bnum = var_bignum_ptr(to);


	return (void *)&to->bnum;
}

static void *
string_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to oct_string\n");
	
	return (void *)&to->octstr;
}

static void *
string_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to string\n");

	return (void *)&to->octstr;
}

static void *
octstring_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to bignum\n");
	
	return (void *)&to->bnum;
}

static void *
octstring_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to oct_string\n");

	return (void *)&to->octstr;
}

static void *
octstring_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to string\n");
	
	return (void *)&to->str;
}

static void *
bignum_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum to bignum\n");

	return (void *)&to->bnum;
}

static void *
bignum_to_octstring(struct variable *to, const struct variable *from)
{
	mp_int *bnum;
	octstr_t *octstr;

	DEBUG(LOG_VERBOSE, "bignum to oct_string\n");

	bnum = var_bignum_ptr((struct variable *)from);
	octstr = var_octstr_ptr(to);
	octstr_reset(octstr);

	return octstr;
}

static void *
bignum_to_string(struct variable *to, const struct variable *from)
{
	mp_int a, b, r;
	mp_int *bnum;
	str_t *str;
	int rc;
	char ch;

	assert(STR_BASE > 2 && STR_BASE < 17);

	DEBUG(LOG_VERBOSE, "bignum ot string\n");

	bnum = var_bignum_ptr((struct variable *)from);
	str = var_str_ptr(to);
	str_reset(str);

	if (mp_iszero(bnum)) {
		str_putc(str, '0');
		return str;
	}

	mp_initv(&a, &b, &r, NULL);

	rc = mp_copy(&a, bnum);
	if (rc != MP_OK)
		goto err;

	mp_set_uint(&b, STR_BASE);

	while (mp_iszero(&a) == 0) {

		rc = mp_div(&a, &r, &a, &b);
		if (rc != MP_OK)
			goto err;
		
		ch = r.dig[0];
		if (ch < 10)
			str_putc(str, ch + '0');
		else
			str_putc(str, ch - 10 + 'A');
	}

	if (mp_isneg(bnum)) {
		str_putc(str, '-');
	}

	str_reverse(str);

	mp_clearv(&a, &b, &r, NULL);

	return str;
err:
	// FIXME: stub
	print_warn_and_die("WIP!\n");
	return str;
}

static int
type_conv_cmp(const void *a, const void *b)
{
	const struct type_conv *pa, *pb;

	pa = a;
	pb = b;

	if (pa->from_type > pb->from_type)
		return 1;
	else if (pa->from_type < pb->from_type)
		return -1;
	else if (pa->to_type > pb->to_type)
		return 1;
	else if (pa->to_type < pb->to_type)
		return -1;

	return 0;
}

