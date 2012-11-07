/*
 * str.h - string routines
 */
#ifndef STRING_H_
#define STRING_H_


typedef struct {
	struct buffer *buf;
} str_t;

void str_init(str_t *str);
void str_initv(str_t *str, ...);

void str_clear(str_t *str);
void str_clearv(str_t *str, ...);

void str_copy(str_t *dst, const str_t *src);
void str_concat(str_t *c, const str_t *a, const str_t *b);
char *str_append(str_t *str, const char *ptr);
char *str_append_n(str_t *str, const char *ptr, size_t n);
char *str_append_str(str_t *dst, const str_t *src);
char *str_snprintf(str_t *str, char *fmt, ...) __attribute__((format(printf,2,3)));
char *str_putc(str_t *str, char c);
char *str_drop(str_t *str, size_t n);
char *str_trim(str_t *str, size_t n);
void str_reset(str_t *str);
size_t str_len(str_t *str);
char *str_ptr(str_t *str);


#endif
