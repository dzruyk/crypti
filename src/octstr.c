#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "buffer.h"
#include "macros.h"
#include "str.h"


void
octstr_init(octstr_t *octstr)
{
	octstr->buf = buffer_new();
}

void
octstr_initv(octstr_t *octstr, ...)
{
	octstr_t *p;

	va_list ap;

	va_start(ap, octstr);

	p = octstr;
	while (p != NULL) {
		octstr_init(p);
		p = va_arg(ap, octstr_t *);
	}
}

void
octstr_clear(octstr_t *octstr)
{
	buffer_free(octstr->buf);
	octstr->buf = NULL;
}

void
octstr_clearv(octstr_t *octstr, ...)
{
	octstr_t *p;

	va_list ap;

	va_start(ap, octstr);

	p = octstr;
	while (p != NULL) {
		octstr_clear(p);
		p = va_arg(ap, octstr_t *);
	}
}

char *
octstr_append(octstr_t *octstr, const char *ptr)
{
	buffer_put(octstr->buf, ptr, octstrlen(ptr));
	return buffer_ptr(octstr->buf);
}

char *
octstr_append_n(octstr_t *octstr, const char *ptr, size_t n)
{
	size_t len;

	len = octstrnlen(ptr, n);
	buffer_put(octstr->buf, ptr, len);
	return buffer_ptr(octstr->buf);
}

char *
octstr_append_octstr(octstr_t *dst, octstr_t *src)
{
	buffer_copy(dst->buf, src->buf, buffer_size(src->buf));
	return buffer_ptr(dst->buf);
}

char *
octstr_putc(octstr_t *octstr, char c)
{
	buffer_putc(octstr->buf, c);
	return buffer_ptr(octstr->buf);
}

#define octstr_SNPRINTF_ADVANCE	4

char *
octstr_snprintf(octstr_t *octstr, char *fmt, ...)
{
	size_t res, new_res;
	va_list ap, ap_copy;

	va_start(ap, fmt);
	va_copy(ap_copy, ap);
	buffer_reset(octstr->buf);
	buffer_advance(octstr->buf, STR_SNPRINTF_ADVANCE);
	res = vsnprintf(buffer_ptr(octstr->buf), STR_SNPRINTF_ADVANCE, fmt, ap);
	if (res < 0)
		error(1, "octstr_snprintf: vsnprintf res=%d", (int)res);
	if (res >= STR_SNPRINTF_ADVANCE) {
		buffer_reset(octstr->buf);
		buffer_advance(octstr->buf, res+1); /* plus one zero char */
		new_res = vsnprintf(buffer_ptr(octstr->buf), res+1, fmt, ap_copy);
		if (new_res < 0 || new_res != res)
			error(1, "octstr_snprintf: should not reach");
	} else
		buffer_trim(octstr->buf, STR_SNPRINTF_ADVANCE-res);
	/* now reduce buffer size down one byte because we have '\0' there */
	buffer_trim(octstr->buf, 1);
	va_end(ap);
	return buffer_ptr(octstr->buf);
}

char *
octstr_drop(octstr_t *octstr, size_t n)
{
	buffer_consume(octstr->buf, n);
	return buffer_ptr(octstr->buf);
}

char *
octstr_trim(octstr_t *octstr, size_t n)
{
	buffer_trim(octstr->buf, n);
	return buffer_ptr(octstr->buf);
}

void
octstr_reset(octstr_t *octstr)
{
	buffer_reset(octstr->buf);
}

size_t
octstr_len(octstr_t *octstr)
{
	return buffer_size(octstr->buf);
}

char *
octstr_ptr(octstr_t *octstr)
{
	return buffer_ptr(octstr->buf);
}

