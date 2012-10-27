#include <stdarg.h>
#include <stdlib.h>
#include <memcpy.h>

#include "common.h"
#include "macros.h"
#include "mp.h"
#include "str.h"
#include "type_convertions.h"
#include "variable.h"

static void var_convert_type(struct variable *var, var_type_t to_type);

void
var_init(struct variable *var)
{
	var->type = 0;

	//FIXME: need to check ret status
	mp_init(var->bnum);
	octstr_init(var->oct_str);
	str_init(var->str);
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
	mp_clear(var->bnum);
	str_clear(var->str);
	octstr_clear(var->oct_str);
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
var_set_string(struct variable *var, str_t str)
{
	var->type |= VAR_STRING;

	str_copy(&var->str, str);
}

void
var_set_str(struct varaible *var, char *str)
{

}

void
var_set_oct_string(struct variable *var, struct oct_string *oct_str)
{
	var->type |= VAR_OCTSTRING;

	octstr_copy(&var->oct_str, oct_str);
}

void
var_set_bignum(struct variable *var, mp_int *bnum)
{
	var->type |= VAR_BIGNUM;

	mp_copy(&var->bnum, bnum);
}

void
var_force_type(struct variable *var, var_type_t type)
{
	
	var->type = type;
}

static void
var_convert_type(struct variable *var, var_type_t to_type)
{
	int from_type;

	//type exist
	if ((var->type & type) != 0)
		return;

	if (var->type & VAR_BIGNUM)
		from_type = VAR_BIGNUM;
	if (var->type & VAR_OCTSTRING)
		from_type = VAR_OCTSTRING;
	if (var->type & VAR_STRING)
		from_type = VAR_STRING;
	
	convert_value(var, to_type, var, from_type);

	var->type |= type;
}

str_t *
var_cast_to_str(struct variable *var)
{
	if ((var->type & VAR_STRING) == 0)
		var_convert_type(var, VAR_STRING);

	return &var->str;
}

ocstr_t *
var_cast_to_octstr(struct variable *var)
{
	if ((var->type & VAR_OCTSTRING) == 0)
		var_convert_type(var, VAR_OCTSTRING);

	return &var->oct_str;
}

mp_int *
var_cast_to_bignum(struct variable *var)
{
	if ((var->type & VAR_BIGNUM) == 0)
		var_convert_type(var, VAR_BIGNUM);

	return &var->bnum;
}


str_t *
var_str_ptr(struct variable *var)
{
	return &var->str;
}

octstr_t *
var_octstr_ptr(struct variable *var)
{
	return &var->oct_str;
}

mp_int *
var_bignum_ptr(struct variable *var)
{
	return &var->bnum;
}
