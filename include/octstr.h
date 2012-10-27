#ifndef __OCTSTRING_H_
#define __OCTSTRING_H_

typedef struct {
	struct buffer *buf;
} octstr_t;

void octstr_init(octstr_t *octstr);
void octstr_initv(octstr_t *octstr, ...);

void octstr_clear(octstr_t *octstr);
void octstr_clearv(octstr_t *octstr, ...);

char *octstr_append(octstr_t *octstr, const char *ptr);
char *octstr_append_n(octstr_t *octstr, const char *ptr, size_t n);
char *octstr_append_octstr(octstr_t *dst, octstr_t *src);
char *octstr_snprintf(octstr_t *octstr, char *fmt, ...) __attribute__((format(printf,2,3)));
char *octstr_putc(octstr_t *octstr, char c);
char *octstr_drop(octstr_t *octstr, size_t n);
char *octstr_trim(octstr_t *octstr, size_t n);
void octstr_reset(octstr_t *octstr);

size_t octstr_len(octstr_t *octstr);
char *octstr_ptr(octstr_t *octstr);
#endif
