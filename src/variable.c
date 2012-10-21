#include <stdlib.h>
#include <memcpy.h>

#include "common.h"
#include "macros.h"
#include "mp.h"
#include "str.h"
#include "type_convertions.h"
#include "variable.h"

static type_converter_t var_find_converter(int from_type, int to_type);

struct variable *
var_init()
{
	struct variable *tmp;

	tmp = xmalloc(sizeof(*tmp));
	memset(tmp, 0, sizeof(*tmp));

	return tmp;
}

void 
var_clean(struct variable *var)
{
	if (var->type & VAR_BIGNUM)
		mp_clear(&(var->bnum));
	if (var->type & VAR_OCTSTRING)
		octstr_free(&var->oct_str);
	if (var->type & VAR_STRING)
		str_free(var->str);
	var->type = VAR_CLEAN;
}

struct variable *
var_copy(struct variable *var)
{
	int types[] = {VAR_BIGNUM, VAR_OCTSTRING, VAR_STRING};
	struct variable *tmp;
	type_converter_t convert;
	
	int i;

	tmp = var_init();
	
	for (i = 0; i < ARRSZ(types); i++) {
		if ((var->type & types[i]) == 0)
			continue;

		convert = var_find_converter(types[i], types[i]);

		convert(tmp, var);

		tmp->type |= types[i];
	}

	return tmp;
}

void 
var_deinit(struct variable *var)
{
	if (var == NULL)
		return;

	var_clean(var);

	ufree(var);
}

void
var_set_string(struct variable *var, str_t str)
{
	var_clean(var);
	var->type = VAR_STRING;

	var->str = str;
}

void
var_set_oct_string(struct variable *var, struct oct_string *oct_str)
{
	var_clean(var);
	var->type = VAR_OCTSTRING;

	var->oct_str =oct_str;
}

void
var_set_bignum(struct variable *var, mp_int *bnum)
{
	var_clean(var);
	var->type = VAR_BIGNUM;

	var->bnum = bnum;
}

void
var_force_type(struct variable *var, var_type_t to_type)
{
	type_converter_t convert;
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
	
	convert = var_find_converter(from_type, to_type);

	convert(var, var);

	var->type |= type;
}

str_t
var_as_str(struct variable *var)
{
	if ((var->type & VAR_STRING) == 0)
		var_force_type(var, VAR_STRING);

	return var->str;
}

struct oct_string *
var_as_oct_str(struct variable *var)
{
	if ((var->type & VAR_OCTSTRING) == 0)
		var_force_type(var, VAR_OCTSTRING);

	return var->oct_str;
}

mp_int *
var_as_bignum(struct variable *var)
{
	if ((var->type & VAR_BIGNUM) == 0)
		var_force_type(var, VAR_BIGNUM);

	return var->bnum;
}


static type_converter_t
var_find_converter(int from_type, int to_type)
{
	struct type_conv conv, *res;
	
	conv.from_type = from_type;
	conv.to_type = to_type;

	res = bsearch(&tmp, morpher, ARRSZ(morpher), sizeof(morpher[0]), type_conv_cmp);

	if (res == NULL)
		print_warn_and_die("can't force type, programmer mistake\n");

	return res->func;
}

