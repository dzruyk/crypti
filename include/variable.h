#ifndef __VARIABLE_H_
#define __VARIABLE_H_

typedef enum {
	VAR_CLEAN = 0;
	VAR_BIGNUM = 0x1;
	VAR_OCTSTRING = 0x2;
	VAR_STRING = 0x4;
} var_type_t;

struct variable {
	var_type_t type;
	str_t str;
	struct oct_string *oct_str;
	mp_int *bnum;
};

struct variable *var_init();

void var_clean(struct variable *var);

struct variable *var_copy(struct variable *var);

void var_deinit(struct variable *var);


void var_set_string(struct variable *var, str_t str);

void var_set_oct_string(struct variable *var, struct oct_string *oct_str);

void var_set_bignum(struct variable *var, mp_int *bnum);

void var_force_type(struct variable *var, var_type_t type);


str_t var_as_str(struct variable *var);

struct oct_string *var_as_oct_str(struct variable *var);

mp_int *var_as_bignum(struct variable *var);

#endif
