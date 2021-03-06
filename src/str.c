/*
 * Dynamic strings
 *
 * Grigoriy Sitkarev, <sitkarev@unixkomi.ru>
 */
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
#include "str.h"

void
str_init(str_t *str)
{
	assert(str != NULL);

	str->buf = buffer_new();
	buffer_zero(str->buf);
}

void
str_initv(str_t *str, ...)
{
	str_t *p;

	va_list ap;

	va_start(ap, str);

	p = str;
	while (p != NULL) {
		str_init(p);
		p = va_arg(ap, str_t *);
	}
}

void
str_clear(str_t *str)
{
	assert(str != NULL);

	buffer_free(str->buf);
	str->buf = NULL;
}

void
str_clearv(str_t *str, ...)
{
	str_t *p;
	va_list ap;

	assert(str != NULL);

	va_start(ap, str);

	p = str;
	while (p != NULL) {
		str_clear(p);
		p = va_arg(ap, str_t *);
	}
}

void
str_copy(str_t *dst, const str_t *src)
{
	assert(src != NULL && dst != NULL
	    &&  src->buf != NULL && dst->buf != NULL);

	buffer_copy(dst->buf, src->buf, buffer_size(src->buf));
}

void
str_concat(str_t *dst, const str_t *a, const str_t *b)
{
	assert(dst != NULL && a != NULL && b != NULL);

	str_copy(dst, a);
	str_append_str(dst, b);
}

void
str_reverse(str_t *str)
{
	char *start, *end;
	char tmp;

	assert(str != NULL);

	start = buffer_ptr(str->buf);
	end = start + buffer_size(str->buf) - 1;

	while (start < end) {
		tmp = *start;
		*start++ = *end;
		*end-- = tmp;
	}
}

int
str_substr(str_t *res, str_t *str, int first, int n)
{
	size_t len;
	int i;

	assert(res != NULL);

	len = str_len(str);

	if (first < 0)
		first = 0;

	if (first > len) {
		str_reset(res);
		return 0;
	}

	if (n > len - first)
		n = len - first;

	str_reset(res);

	for (i = 0; i < n; i++) {
		char ch;
		ch = buffer_get_u8_at(str->buf, first + i);
		str_putc(res, ch);
	}

	return 0;
}

char *
str_append(str_t *str, const char *ptr)
{
	assert(str != NULL && ptr != NULL);

	buffer_put(str->buf, ptr, strlen(ptr));
	buffer_zero(str->buf);
	return buffer_ptr(str->buf);
}

char *
str_append_n(str_t *str, const char *ptr, size_t n)
{
	size_t len;

	assert(str != NULL);

	len = strnlen(ptr, n);
	buffer_put(str->buf, ptr, len);
	buffer_zero(str->buf);
	return buffer_ptr(str->buf);
}

char *
str_append_str(str_t *dst, const str_t *src)
{
	assert(src != NULL && dst != NULL);

	buffer_append(dst->buf, src->buf, buffer_size(src->buf));
	buffer_zero(dst->buf);
	return buffer_ptr(dst->buf);
}

char *
str_putc(str_t *str, char c)
{
	assert(str != NULL);

	buffer_putc(str->buf, c);
	buffer_zero(str->buf);

	return buffer_ptr(str->buf);
}

#define STR_SNPRINTF_ADVANCE	4

char *
str_snprintf(str_t *str, char *fmt, ...)
{
	assert(str != NULL && fmt != NULL);

	size_t res, new_res;
	va_list ap, ap_copy;

	va_start(ap, fmt);
	va_copy(ap_copy, ap);
	buffer_reset(str->buf);
	buffer_advance(str->buf, STR_SNPRINTF_ADVANCE);
	res = vsnprintf(buffer_ptr(str->buf), STR_SNPRINTF_ADVANCE, fmt, ap);
	if (res < 0)
		error(1, "str_snprintf: vsnprintf res=%d", (int)res);
	if (res >= STR_SNPRINTF_ADVANCE) {
		buffer_reset(str->buf);
		buffer_advance(str->buf, res+1); /* plus one zero char */
		new_res = vsnprintf(buffer_ptr(str->buf), res+1, fmt, ap_copy);
		if (new_res < 0 || new_res != res)
			error(1, "str_snprintf: should not reach");
	} else
		buffer_trim(str->buf, STR_SNPRINTF_ADVANCE - res);
	/* now reduce buffer size down one byte because we have '\0' there */
	buffer_trim(str->buf, 1);
	va_end(ap);
	return buffer_ptr(str->buf);
}

char *
str_drop(str_t *str, size_t n)
{
	assert(str != NULL);

	buffer_consume(str->buf, n);
	return buffer_ptr(str->buf);
}

char *
str_trim(str_t *str, size_t n)
{
	assert(str != NULL);

	buffer_trim(str->buf, n);
	buffer_zero(str->buf);
	return buffer_ptr(str->buf);
}

void
str_reset(str_t *str)
{
	assert(str != NULL);

	buffer_reset(str->buf);
	buffer_zero(str->buf);
}

size_t
str_len(str_t *str)
{
	assert(str != NULL);

	return buffer_size(str->buf);
}

char *
str_ptr(str_t *str)
{
	assert(str != NULL);

	return buffer_ptr(str->buf);
}

