#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "macros.h"
#include <mpl.h>
#include "octstr.h"
#include "str.h"
#include "variable.h"
#include "type_convertions.h"

static void *var_convert_type(struct variable *var, var_type_t to_type);

void
var_init(struct variable *var)
{
	assert(var != NULL);

	var->type = 0;

	//FIXME: need to check ret status
	mpl_init(&var->bnum);
	octstr_init(&var->octstr);
	str_init(&var->str);
}

void
var_initv(struct variable *var, ...)
{
	struct variable *p;

	va_list ap;

	va_start(ap, var);
	p = var;

	while (p != NULL) {
		var_init(p);
		p = va_arg(ap, struct variable *);
	}
	va_end(ap);
}

void
var_clear(struct variable *var)
{
	assert(var != NULL);

	mpl_clear(&var->bnum);
	str_clear(&var->str);
	octstr_clear(&var->octstr);
}

void
var_clearv(struct variable *var, ...)
{
	struct variable *p;

	va_list ap;

	va_start(ap, var);
	p = var;

	while (p != NULL) {
		var_clear(p);
		p = va_arg(ap, struct variable *);
	}
	va_end(ap);
}

void
var_copy(struct variable *dst, struct variable *src)
{
	int types[] = {VAR_BIGNUM, VAR_OCTSTRING, VAR_STRING};
	int i;

	assert(dst != NULL && src != NULL);

	dst->type = 0;

	for (i = 0; i < ARRSZ(types); i++) {
		if ((src->type & types[i]) == 0)
			continue;
		convert_value(dst, types[i], src, types[i]);

		dst->type |= types[i];
	}
	DEBUG(LOG_VERBOSE, "after copy dst types %.4x\n", dst->type);
}

void
var_set_string(struct variable *var, str_t *str)
{
	assert(var != NULL && str != NULL);

	var->type = VAR_STRING;
	str_copy(&var->str, str);
}

void
var_set_str(struct variable *var, char *str)
{
	assert(var != NULL && str != NULL);

	var->type = VAR_STRING;

	str_reset(&var->str);
	str_append(&var->str, str);
}

void
var_set_octstr(struct variable *var, octstr_t *octstr)
{
	assert(var != NULL && octstr != NULL);

	var->type = VAR_OCTSTRING;
	octstr_copy(&var->octstr, octstr);
}

void
var_set_bignum(struct variable *var, mpl_int *bnum)
{
	assert(var != NULL && bnum != NULL);

	var->type = VAR_BIGNUM;
	mpl_copy(&var->bnum, bnum);
}

void
var_set_int(struct variable *var, int num)
{
	assert(var != NULL);

	var->type = VAR_BIGNUM;
	mpl_set_sint(&var->bnum, num);
}

void
var_set_one(struct variable *var)
{
	assert(var != NULL);

	var->type = VAR_BIGNUM;
	mpl_set_one(&var->bnum);
}

void
var_set_zero(struct variable *var)
{
	assert(var != NULL);

	var->type = VAR_BIGNUM;
	mpl_zero(&var->bnum);

}

void
var_force_type(struct variable *var, var_type_t type)
{
	var->type = type;
}

static void *
var_convert_type(struct variable *var, var_type_t to_type)
{
	int from_type;

	assert(var != NULL && (
	    to_type == VAR_BIGNUM ||
	    to_type == VAR_STRING ||
	    to_type == VAR_OCTSTRING));

	if (var->type != VAR_BIGNUM &&
	    var->type != VAR_OCTSTRING &&
	    var->type != VAR_STRING)
		printf("composite type");

	//type exist
	if ((var->type & to_type) != 0) {
		if (var->type & VAR_BIGNUM)
			return &var->bnum;
		if (var->type & VAR_OCTSTRING)
			return &var->octstr;
		if (var->type & VAR_STRING)
			return &var->str;
	}

	if (var->type & VAR_BIGNUM)
		from_type = VAR_BIGNUM;
	else if (var->type & VAR_OCTSTRING)
		from_type = VAR_OCTSTRING;
	else if (var->type & VAR_STRING)
		from_type = VAR_STRING;

	var->type |= to_type;

	return convert_value(var, to_type, var, from_type);
}

str_t *
var_cast_to_str(struct variable *var)
{
	assert(var != NULL);

	return (str_t *)var_convert_type(var, VAR_STRING);
}

octstr_t *
var_cast_to_octstr(struct variable *var)
{
	assert(var != NULL);

	return (octstr_t *)var_convert_type(var, VAR_OCTSTRING);
}

mpl_int *
var_cast_to_bignum(struct variable *var)
{
	assert(var != NULL);

	return (mpl_int *)var_convert_type(var, VAR_BIGNUM);
}


str_t *
var_str_ptr(struct variable *var)
{
	assert(var != NULL);

	return &var->str;
}

octstr_t *
var_octstr_ptr(struct variable *var)
{
	assert(var != NULL);

	return &var->octstr;
}

mpl_int *
var_bignum_ptr(struct variable *var)
{
	assert(var != NULL);

	return &var->bnum;
}

