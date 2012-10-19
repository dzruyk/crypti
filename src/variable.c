#include <memcpy.h>

#include "common.h"
#include "variable.h"


struct variable *
var_init()
{
	struct variable *tmp;

	tmp = xmalloc(sizeof(*tmp));
	memset(tmp, 0, sizeof(*tmp));

	return tmp;
}

struct variable *
var_copy(struct variable *var)
{

}

void var_deinit(struct variable *var)
{
	if (var == NULL)
		return;

	if (var->type & VAR_BIGNUM)
	if (var->type & VAR_OCTSTRING)
	if (var->type & VAR_STRING)
		ufree(var->str);
	ufree(var);
}


void var_set_string(struct variable *var, char *str)
{

}

void var_set_oct_string(struct variable *var, struct oct_string *oct_str)
{

}

void var_set_bignum(struct variable *var, mp_int *bnum)
{

}

void var_force_type(struct variable *var, var_type_t type)
{

}

char *var_as_str(struct variable *var)
{

}

struct oct_string *var_as_oct_str(struct variable *var)
{

}

mp_int *var_as_bignum(struct variable *var)
{

}


