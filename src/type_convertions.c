#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <limits.h>

#include "common.h"
#include "log.h"
#include "macros.h"
#include "mp.h"
#include "str.h"
#include "octstr.h"
#include "variable.h"
#include "type_convertions.h"

/* FIXME: Need to set as global variable. */

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

	assert(dst_var != NULL && src_var != NULL);

	conv.from_type = from_type;
	conv.to_type = to_type;

	res = bsearch(&conv, func_table, ARRSZ(func_table), sizeof(func_table[0]), type_conv_cmp);

	if (res == NULL)
		print_warn_and_die("can't force type, programmer mistake (from %d to %d)\n", from_type, to_type);

	return res->func(dst_var, src_var);
}

static void *
string_to_bignum(struct variable *to, const struct variable *from)
{
	mp_int *bnum;
	str_t *str;
	char *src;
	int rc;

	DEBUG(LOG_VERBOSE, "string to bignum\n");

	str = var_str_ptr((struct variable *)from);
	bnum = var_bignum_ptr(to);

	src = str_ptr(str);
	rc = mp_set_str(bnum, src, STR_BASE);
	if (rc != MP_OK)
		error(1, "mp_set_str error");
	
	return bnum;
}

static void *
string_to_octstring(struct variable *to, const struct variable *from)
{
	str_t *str;
	octstr_t *octstr;
	char *src;
	int i, len;

	DEBUG(LOG_VERBOSE, "string to oct_string\n");
	
	str = var_str_ptr((struct variable *)from);
	octstr = var_octstr_ptr(to);

	octstr_reset(octstr);

	len = str_len(str);
	src = str_ptr(str);

	for (i = 0; i < len; i++) {
		char ch;
		ch = src[i];
		if (isprint(ch)) {
			str_putc(str, ch);
		} else {
			char ccode[5];
			
			snprintf(ccode, sizeof(ccode), "\\x%.2X", ch);
			str_append(str, ccode);
		}
	}

	return octstr;
}

static void *
string_to_string(struct variable *to, const struct variable *from)
{
	str_t *dst, *src;

	DEBUG(LOG_VERBOSE, "string to string\n");
	
	if (to == from) {
		DEBUG(LOG_DEFAULT, "same octstring used in convertion\n");
		return (void *)&to->str;
	}

	src = var_str_ptr((struct variable *)from);
	dst = var_str_ptr(to);

	str_copy(dst, src);

	return dst;
}

static void *
octstring_to_bignum(struct variable *to, const struct variable *from)
{
	mp_int *bnum;
	octstr_t *octstr;
	unsigned char *src;
	int rc, sz;

	DEBUG(LOG_VERBOSE, "oct_string to bignum\n");

	octstr = var_octstr_ptr((struct variable *)from);
	bnum = var_bignum_ptr(to);

	src = octstr_ptr(octstr);
	sz = octstr_len(octstr);

	rc = mp_set_uchar(bnum, (const unsigned char *)src, sz);
	if (rc != MP_OK)
		error(1, "mp_set_uchar error\n");

	return bnum;
}

static void *
octstring_to_octstring(struct variable *to, const struct variable *from)
{
	octstr_t *dst, *src;

	DEBUG(LOG_VERBOSE, "oct_string to oct_string\n");
	
	if (to == from) {
		DEBUG(LOG_DEFAULT, "same octstring used in convertion\n");
		return (void *)&to->octstr;
	}

	src = var_octstr_ptr((struct variable *)from);
	dst = var_octstr_ptr(to);

	octstr_copy(dst, src);

	return dst;
}

/*
 * FIXME:
 * now use fat snprintf version, write light version of itoa;
 */
static void *
octstring_to_string(struct variable *to, const struct variable *from)
{
	octstr_t *octstr;
	str_t *str;
	unsigned char *src;
	int i, len;

	DEBUG(LOG_VERBOSE, "oct_string to string\n");

	octstr = var_octstr_ptr((struct variable *)from);
	str = var_str_ptr(to);

	str_reset(str);

	len = octstr_len(octstr);
	src = octstr_ptr(octstr);

	for (i = 0; i < len; i++) {
		char ch;
		ch = src[i];
		if (isprint(ch)) {
			str_putc(str, ch);
		} else {
			char ccode[5];
			
			snprintf(ccode, sizeof(ccode), "\\x%.2X", ch);
			str_append(str, ccode);
		}
	}

	return str;
}

static void *
bignum_to_bignum(struct variable *to, const struct variable *from)
{
	mp_int *dst, *src;
	int rc;

	DEBUG(LOG_VERBOSE, "bignum to bignum\n");
	
	if (to == from) {
		DEBUG(LOG_DEFAULT, "same bignum used in convertion\n");
		return (void *)&to->bnum;
	}

	src = var_bignum_ptr((struct variable *)from);
	dst = var_bignum_ptr(to);

	rc = mp_copy(dst, src);
	if (rc != MP_OK)
		goto err;

	return dst;

err:
	error(1, "convertion fail");
}

/* 
 * WARN:
 * can produce problem with byte order
 */
static void *
bignum_to_octstring(struct variable *to, const struct variable *from)
{
	mp_int *bnum;
	_mp_int_t *dig;
	octstr_t *octstr;
	int bytes, n;

	DEBUG(LOG_VERBOSE, "bignum to oct_string\n");

	bnum = var_bignum_ptr((struct variable *)from);
	octstr = var_octstr_ptr(to);
	octstr_reset(octstr);

	if (mp_isneg(bnum))
		warning("neg bnum to str convertion: sign mismatch\n");
	
	if (mp_iszero(bnum)) {
		octstr_putc(octstr, '\x00');
		return octstr;
	}

	n = mp_nr_bits(bnum);

	//get byte size of used space
	bytes = n / CHAR_BIT;
	if (n % CHAR_BIT != 0)
		bytes++;

	dig = bnum->dig;

	octstr_append_n(octstr, dig, bytes);

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

