/*
 * str.h - string routines
 */
#ifndef STRING_H_
#define STRING_H_

typedef struct buffer * str_t;

#define str_ptr(str)	((char *)(buffer_ptr((struct buffer*)str)))

str_t str_new();
void str_free(str_t *str);
char *str_append(str_t str, const char *ptr);
char *str_append_n(str_t str, const char *ptr, size_t n);
char *str_append_str(str_t dst, str_t src);
char *str_snprintf(str_t str, char *fmt, ...) __attribute__((format(printf,2,3)));
char *str_putc(str_t str, char c);
char *str_drop(str_t str, size_t n);
char *str_trim(str_t str, size_t n);
void str_reset(str_t str);
size_t str_len(str_t str);


#endif
