#ifndef __VAR_OP_H__
#define __VAR_OP_H__

int varop_copy(struct variable *dst, struct variable *src);

/* Number operations: */
int varop_add(struct variable *c, struct variable *a, struct variable *b);
int varop_sub(struct variable *c, struct variable *a, struct variable *b);
int varop_mul(struct variable *c, struct variable *a, struct variable *b);
int varop_div(struct variable *c, struct variable *a, struct variable *b);
int varop_mod(struct variable *c, struct variable *a, struct variable *b);
int varop_pow(struct variable *c, struct variable *a, struct variable *b);
int varop_gcd(struct variable *c, struct variable *a, struct variable *b);
int varop_mod_inv(struct variable *c, struct variable *a, struct variable *b);
int varop_mod_exp(struct variable *res, struct variable *a,
    struct variable *y, struct variable *b);

/* Octstr operations: */
int varop_or(struct variable *c, struct variable *a, struct variable *b);
int varop_xor(struct variable *c, struct variable *a, struct variable *b);
int varop_and(struct variable *c, struct variable *a, struct variable *b);

int varop_not(struct variable *res, struct variable *var);
int varop_neg(struct variable *res, struct variable *var);

int varop_shl(struct variable *c, struct variable *a, struct variable *b);
int varop_shr(struct variable *c, struct variable *a, struct variable *b);

/* String operations: */
int varop_str_concat(struct variable *c, struct variable *a, struct variable *b);
int varop_str_sub(struct variable *res, struct variable *s, struct variable *start, struct variable *len);
int varop_str_len(struct variable *res, struct variable *a);

/* Octstring operations: */
int varop_lpad(struct variable *dst, struct variable *src, struct variable *width, struct variable *filler);
int varop_rpad(struct variable *dst, struct variable *src, struct variable *width, struct variable *filler);
int varop_oct_concat(struct variable *c, struct variable *a, struct variable *b);
int varop_octstr_sub(struct variable *res, struct variable *s, struct variable *start, struct variable *len);
int varop_octstr_len(struct variable *res, struct variable *a);

/* Rel operations.
 * return:
 * 1 if a > b
 * -1 if b < a
 *  0 if a == b
 */
int varop_cmp(struct variable *a, struct variable *b);
int varop_is_true(struct variable *a);

#endif
