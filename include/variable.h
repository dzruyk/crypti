#ifndef __VARIABLE_H_
#define __VARIABLE_H_

/* base of printable string */
#define STR_BASE 10

#include <mpl.h>
#include "octstr.h"
#include "str.h"

typedef enum {
	VAR_BIGNUM = 0x1,
	VAR_OCTSTRING = 0x2,
	VAR_STRING = 0x4,
} var_type_t;

struct variable {
	var_type_t type;
	mpl_int bnum;
	octstr_t octstr;
	str_t str;
};

void var_init(struct variable *var);
/* Init null terminated list of variables. */
void var_initv(struct variable *var, ...);
void var_clear(struct variable *var);
/* clear null terminated list of variables. */
void var_clearv(struct variable *var, ...);

void var_copy(struct variable *dst, struct variable *src);

void var_set_string(struct variable *var, str_t *str);
void var_set_str(struct variable *var, char *str);
void var_set_octstr(struct variable *var, octstr_t *octstr);
void var_set_bignum(struct variable *var, mpl_int *bnum);
void var_set_int(struct variable *var, int num);

/* some setters here */
void var_set_one(struct variable *var);
void var_set_zero(struct variable *var);

void var_force_type(struct variable *var, var_type_t type);

/* NOTE: Function always succeeds */
str_t *var_cast_to_str(struct variable *var);
octstr_t *var_cast_to_octstr(struct variable *var);
mpl_int *var_cast_to_bignum(struct variable *var);

str_t *var_str_ptr(struct variable *var);
octstr_t *var_octstr_ptr(struct variable *var);
mpl_int *var_bignum_ptr(struct variable *var);

#endif
