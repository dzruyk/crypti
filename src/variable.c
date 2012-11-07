#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "macros.h"
#include "mp.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"
#include "type_convertions.h"

static void var_convert_type(struct variable *var, var_type_t to_type);

void
var_init(struct variable *var)
{
	assert(var != NULL);

	var->type = 0;

	//FIXME: need to check ret status
	mp_init(&var->bnum);
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
}

void
var_clear(struct variable *var)
{
	assert(var != NULL);

	mp_clear(&var->bnum);
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
}

void
var_copy(struct variable *dst, struct variable *src)
{
	assert(dst != NULL && src != NULL);

	int types[] = {VAR_BIGNUM, VAR_OCTSTRING, VAR_STRING};
	
	int i;

	dst->type = 0;

	for (i = 0; i < ARRSZ(types); i++) {
		if ((src->type & types[i]) == 0)
			continue;

		convert_value(dst, types[i], src, types[i]);

		dst->type |= types[i];
	}
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

	str_reset(var->str);
	str_append(var->str, str);
}

void
var_set_octstr(struct variable *var, octstr_t *octstr)
{
	assert(var != NULL && octstr != NULL);

	var->type = VAR_OCTSTRING;

	octstr_copy(&var->octstr, octstr);
}

void
var_set_bignum(struct variable *var, mp_int *bnum)
{
	assert(var != NULL && bnum != NULL);

	var->type = VAR_BIGNUM;

	mp_copy(&var->bnum, bnum);
}

void
var_force_type(struct variable *var, var_type_t type)
{
	assert(var != NULL && ((var->type & type) != 0));

	var->type = type;
}

static void
var_convert_type(struct variable *var, var_type_t to_type)
{
	assert(var != NULL && (
	    type == VAR_BIGNUM ||
	    type == VAR_STRING ||
	    type == VAR_OCTSTRING);
	
	int from_type;

	//type exist
	if ((var->type & to_type) != 0)
		return;

	if (var->type & VAR_BIGNUM)
		from_type = VAR_BIGNUM;
	if (var->type & VAR_OCTSTRING)
		from_type = VAR_OCTSTRING;
	if (var->type & VAR_STRING)
		from_type = VAR_STRING;
	
	convert_value(var, to_type, var, from_type);

	var->type |= to_type;
}

str_t *
var_cast_to_str(struct variable *var)
{
	assert(var != NULL);

	if ((var->type & VAR_STRING) == 0)
		var_convert_type(var, VAR_STRING);

	return &var->str;
}

octstr_t *
var_cast_to_octstr(struct variable *var)
{
	assert(var != NULL);

	if ((var->type & VAR_OCTSTRING) == 0)
		var_convert_type(var, VAR_OCTSTRING);

	return &var->octstr;
}

mp_int *
var_cast_to_bignum(struct variable *var)
{
	assert(var != NULL);

	if ((var->type & VAR_BIGNUM) == 0)
		var_convert_type(var, VAR_BIGNUM);

	return &var->bnum;
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

mp_int *
var_bignum_ptr(struct variable *var)
{
	assert(var != NULL);

	return &var->bnum;
}

