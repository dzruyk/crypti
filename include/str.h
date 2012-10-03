#ifndef __STR_H__
#define __STR_H__

typedef struct {
	int len;
	unsigned char *buff;
} str_t;

/*
 * create new string, fill it with len bytes
 * from ptr
 */
str_t *str_new(void *ptr, int len);

/*
 * create new string with len bytes size,
 * zeroes all bytes
 */
str_t *str_new_clean(int len);

/*
 * return length of string
 */
int str_len(str_t *str);

/*
 * resize string to str size
 */
void str_resize(str_t *str, int sz);

/*
 * return result of concantenation left and right strings
 */
str_t *str_concat(str_t *left, str_t *right);

/*
 * duplicate passed string
 */
str_t *str_dup(str_t *str);

/*
 * return new substring of str
 * NOTE:
 * first must be positive or 0
 * len will cutoff to str->len - first
 */
str_t *str_sub(str_t *str, int first, int len);

/*
 * Print every character in hexadecimal form
 */
void str_print(str_t *str);

/*
 * Delete string
 */
void str_del(str_t *str);

#endif
