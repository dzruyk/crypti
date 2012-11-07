#include <assert.h>
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
#include "octstr.h"

/* FIXME:
 * Rly need to return char ptr?
 * Remember that octstring is not NULL terminated!
 */
void
octstr_init(octstr_t *octstr)
{
	assert(octstr != NULL);

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
	assert(octstr != NULL);

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

void
octstr_copy(octstr_t *dst, const octstr_t *src)
{
	assert(src != NULL && dst != NULL
	    &&  src->buf != NULL && dst->buf != NULL);

	buffer_copy(dst->buf, src->buf, buffer_size(src->buf));
}

void
octstr_concat(octstr_t *dst, const octstr_t *a, const octstr_t *b)
{
	octstr_copy(dst, a);
	octstr_append_octstr(dst, b);
}

char *
octstr_append(octstr_t *octstr, const char *ptr)
{
	assert(octstr != NULL);

	buffer_put(octstr->buf, ptr, strlen(ptr));
	return buffer_ptr(octstr->buf);
}

char *
octstr_append_n(octstr_t *octstr, const char *ptr, size_t n)
{
	assert(octstr != NULL);

	buffer_put(octstr->buf, ptr, n);
	return buffer_ptr(octstr->buf);
}

char *
octstr_append_octstr(octstr_t *dst, const octstr_t *src)
{
	assert(dst != NULL && src != NULL);

	buffer_append(dst->buf, src->buf, buffer_size(src->buf));
	return buffer_ptr(dst->buf);
}

char *
octstr_putc(octstr_t *octstr, char c)
{
	assert(octstr != NULL);

	buffer_putc(octstr->buf, c);
	return buffer_ptr(octstr->buf);
}

#define OCTSTR_SNPRINTF_ADVANCE	4

char *
octstr_snprintf(octstr_t *octstr, char *fmt, ...)
{
	assert(octstr != NULL);

	size_t res, new_res;
	va_list ap, ap_copy;

	va_start(ap, fmt);
	va_copy(ap_copy, ap);
	buffer_reset(octstr->buf);
	buffer_advance(octstr->buf, OCTSTR_SNPRINTF_ADVANCE);
	res = vsnprintf(buffer_ptr(octstr->buf), OCTSTR_SNPRINTF_ADVANCE, fmt, ap);
	if (res < 0)
		error(1, "octstr_snprintf: vsnprintf res=%d", (int)res);
	if (res >= OCTSTR_SNPRINTF_ADVANCE) {
		buffer_reset(octstr->buf);
		buffer_advance(octstr->buf, res+1); /* plus one zero char */
		new_res = vsnprintf(buffer_ptr(octstr->buf), res+1, fmt, ap_copy);
		if (new_res < 0 || new_res != res)
			error(1, "octstr_snprintf: should not reach");
	} else
		buffer_trim(octstr->buf, OCTSTR_SNPRINTF_ADVANCE - res);
	/* now reduce buffer size down one byte because we have '\0' there */
	buffer_trim(octstr->buf, 1);
	va_end(ap);
	return buffer_ptr(octstr->buf);
}

char *
octstr_drop(octstr_t *octstr, size_t n)
{
	assert(octstr != NULL);

	buffer_consume(octstr->buf, n);
	return buffer_ptr(octstr->buf);
}

char *
octstr_trim(octstr_t *octstr, size_t n)
{
	assert(octstr != NULL);

	buffer_trim(octstr->buf, n);
	return buffer_ptr(octstr->buf);
}

void
octstr_reset(octstr_t *octstr)
{
	assert(octstr != NULL);

	buffer_reset(octstr->buf);
}

size_t
octstr_len(octstr_t *octstr)
{
	assert(octstr != NULL);

	return buffer_size(octstr->buf);
}

char *
octstr_ptr(octstr_t *octstr)
{
	return buffer_ptr(octstr->buf);
}

