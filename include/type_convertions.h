#ifndef __TYPE_CONVERTIONS_H_
#define __TYPE_CONVERTIONS_H_

void string_to_bignum(struct variable *to, const struct variable *from);
void string_to_octstring(struct variable *to, const struct variable *from);
void string_to_string(struct variable *to, const struct variable *from);

void octstring_to_bignum(struct variable *to, const struct variable *from);
void octstring_to_octstring(struct variable *to, const struct variable *from);
void octstring_to_string(struct variable *to, const struct variable *from);

void bignum_to_bignum(struct variable *to, const struct variable *from);
void bignum_to_octstring(struct variable *to, const struct variable *from);
void bignum_to_string(struct variable *to, const struct variable *from);

int type_conv_cmp(const void *a, const void *b);

typedef (*type_converter_t)(struct variable *to, const struct variable *from);

struct type_conv {
	int from_type;
	int to_type;
	type_converter_t func;
};

extern struct type_conv morpher;

#endif
