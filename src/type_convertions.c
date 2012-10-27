
#include "common.h"
#include "crypti.h"
#include "mp.h"
#include "str.h"
#include "type_convertions.h"
#include "variable.h"

static void string_to_bignum(struct variable *to, const struct variable *from);
static void string_to_octstring(struct variable *to, const struct variable *from);
static void string_to_string(struct variable *to, const struct variable *from);

static void octstring_to_bignum(struct variable *to, const struct variable *from);
static void octstring_to_octstring(struct variable *to, const struct variable *from);
static void octstring_to_string(struct variable *to, const struct variable *from);

static void bignum_to_bignum(struct variable *to, const struct variable *from);
static void bignum_to_octstring(struct variable *to, const struct variable *from);
static void bignum_to_string(struct variable *to, const struct variable *from);

static int type_conv_cmp(const void *a, const void *b);

typedef (*type_converter_t)(struct variable *to, const struct variable *from);

struct type_conv {
	int from_type;
	int to_type;
	type_converter_t func;
};

struct type_conv func_table[] = {
	{VAR_BIGNUM, VAR_BIGNUM, bignum_to_bignum},
	{VAR_BIGNUM, VAR_OCTSTRING, bignum_to_octstring},
	{VAR_BIGNUM, VAR_STRING, bignum_to_string},
	
	{VAR_OCTSTRING, VAR_BIGNUM, octstring_to_bignum},
	{VAR_OCTSTRING, VAR_OCTSTRING, octstring_to_octstring},
	{VAR_OCTSTRING, VAR_STRING, octstring_to_string},

	{VAR_STRING, VAR_BIGNUM, string_to_bignum},
	{VAR_STRING, VAR_OCTSTRING, string_to_octstring},
	{VAR_STRING, VAR_STRING, string_to_string},
};

void convert_value(struct variable *dst_var, int to_type, struct variable *src_var, int from_type)
{
	struct type_conv conv, *res;

	conv.from_type = from_type;
	conv.to_type = to_type;

	res = bsearch(&tmp, func_table, ARRSZ(func_table), sizeof(func_table[0]), type_conv_cmp);

	if (res == NULL)
		print_warn_and_die("can't force type, programmer mistake\n");
	res->func(dst_var, src_var);
}

static void
string_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to bignum\n");
}

static void
string_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to oct_string\n");

}

static void
string_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to string\n");
}

static void
octstring_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to bignum\n");

}

static void
octstring_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to oct_string\n");
}

static void
octstring_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to string\n");

}

static void
bignum_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum to bignum\n");
}

static void
bignum_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum to oct_string\n");
}

static void
bignum_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum ot string\n");
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

