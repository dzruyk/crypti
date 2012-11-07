#ifndef __VAR_OP_H__
#define __VAR_OP_H__

int varop_copy(struct variable *dst, struct variable *src);

/* Number operations: */
int varop_add(struct variable *c, struct variable *a, struct variable *b);
int varop_sub(struct variable *c, struct variable *a, struct variable *b);
int varop_mul(struct variable *c, struct variable *a, struct variable *b);
int varop_div(struct variable *c, struct variable *a, struct variable *b);
int varop_pow(struct variable *c, struct variable *a, struct variable *b);
int varop_gcd(struct variable *c, struct variable *a, struct variable *b);

int varop_or(struct variable *c, struct variable *a, struct variable *b);
int varop_xor(struct variable *c, struct variable *a, struct variable *b);
int varop_and(struct variable *c, struct variable *a, struct variable *b);

int varop_cmp(struct variable *dst, struct variable *a, struct variable *b);
int varop_not(struct variable *dst, struct variable *src);
int varop_neg(struct variable *res, struct variable *var);

int varop_shl(struct variable *c, struct variable *a, struct variable *b);
int varop_shr(struct variable *c, struct variable *a, struct variable *b);

/* String operations: */
int varop_str_concat(struct variable *c, struct variable *a, struct variable *b);

/* Octstring operations: */
int varop_oct_concat(struct variable *c, struct variable *a, struct variable *b);

#endif
