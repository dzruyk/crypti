#ifndef __TYPE_CONVERTIONS_H_
#define __TYPE_CONVERTIONS_H_

void *string_to_bignum(struct variable *to, const struct variable *from);
void *string_to_oct_string(struct variable *to, const struct variable *from);

void *oct_string_to_bignum(struct variable *to, const struct variable *from);
void *oct_string_to_string(struct variable *to, const struct variable *from);

void *bignum_to_oct_string(struct variable *to, const struct variable *from);
void *bignum_to_string(struct variable *to, const struct variable *from);

struct type_conv {
	int from_type;
	int to_type;
	void (*func)(struct variable *to, const struct variable *from);
};

#endif
