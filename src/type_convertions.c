
#include "common.h"
#include "crypti.h"
#include "mp.h"
#include "str.h"
#include "type_convertions.h"
#include "variable.h"

struct type_conv morpher[] = {
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

void
string_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to bignum\n");
}

void
string_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to oct_string\n");

}


void
string_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "string to string\n");
}

void
octstring_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to bignum\n");

}

void
octstring_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to oct_string\n");
}

void
octstring_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "oct_string to string\n");

}

void
bignum_to_bignum(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum to bignum\n");
}

void
bignum_to_octstring(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum to oct_string\n");
}

void
bignum_to_string(struct variable *to, const struct variable *from)
{
	DEBUG(LOG_VERBOSE, "bignum ot string\n");
}

int
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

