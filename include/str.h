#ifndef __STR_H__
#define __STR_H__

typedef struct {
	int len;
	void *ptr;
} str_t;


str_t *str_new(int len);

int str_len(str_t *str);

str_t *str_resize(str_t *str, int sz);

str_t *str_concat(str_t *left, str_t *right);

str_t *str_dup(str_t *str);

str_t *str_sub(str_t *str);

void str_print(str_t *str);

void str_del(str_t *str);

#endif
