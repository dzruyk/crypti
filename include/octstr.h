#ifndef __OCTSTRING_H_
#define __OCTSTRING_H_

typedef struct {
	struct buffer *buf;
} octstr_t;

void octstr_init(octstr_t *octstr);
void octstr_initv(octstr_t *octstr, ...);

void octstr_clear(octstr_t *octstr);
void octstr_clearv(octstr_t *octstr, ...);

void octstr_copy(octstr_t *dst, const octstr_t *src);
void octstr_concat(octstr_t *dst, const octstr_t *a, const octstr_t *b);

void octstr_or(octstr_t *dst, const octstr_t *a, const octstr_t *b);
void octstr_xor(octstr_t *dst, const octstr_t *a, const octstr_t *b);
void octstr_and(octstr_t *dst, const octstr_t *a, const octstr_t *b);

int octstr_substr(octstr_t *res, octstr_t *str, int first, int n);
char *octstr_append(octstr_t *octstr, const char *ptr);
char *octstr_append_n(octstr_t *octstr, const void *ptr, size_t n);
char *octstr_append_octstr(octstr_t *dst, const octstr_t *src);
char *octstr_snprintf(octstr_t *octstr, char *fmt, ...) __attribute__((format(printf,2,3)));
char *octstr_putc(octstr_t *octstr, char c);
char *octstr_drop(octstr_t *octstr, size_t n);
char *octstr_trim(octstr_t *octstr, size_t n);
void octstr_reset(octstr_t *octstr);

size_t octstr_len(const octstr_t *octstr);
unsigned char *octstr_ptr(octstr_t *octstr);

#endif
