#ifndef __TYPE_CONVERTIONS_H_
#define __TYPE_CONVERTIONS_H_

void *string_to_bignum(void *val);
void *string_to_oct_string(void *val);

void *oct_string_to_bignum(void *val);
void *oct_string_to_string(void *val);

void *bignum_to_oct_string(void *val);
void *bignum_to_string(void *val);

struct type_conv {
	int from_type;
	int to_type;
	void (*func)(struct value *to, const struct value *from);
};

#endif
