/*
 * Dynamic strings
 *
 * Grigoriy Sitkarev, <sitkarev@unixkomi.ru>
 */
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

str_t str_new()
{
	struct buffer *buf;

	buf = buffer_new();
	buffer_zero(buf);
	return buf;
}

void str_free(str_t *str)
{
	buffer_free(*str);
	*str = NULL;
}

char *str_append(str_t str, const char *ptr)
{
	buffer_put(str, ptr, strlen(ptr));
	buffer_zero(str);
	return buffer_ptr(str);
}

char *str_append_n(str_t str, const char *ptr, size_t n)
{
	size_t len;

	len = strnlen(ptr, n);
	buffer_put(str, ptr, len);
	buffer_zero(str);
	return buffer_ptr(str);
}

char *str_append_str(str_t dst, str_t src)
{
	buffer_copy(dst, src, buffer_size(src));
	buffer_zero(dst);
	return buffer_ptr(dst);
}

char *str_putc(str_t str, char c)
{
	buffer_putc(str, c);
	buffer_zero(str);
	return buffer_ptr(str);
}

#define STR_SNPRINTF_ADVANCE	4

char *str_snprintf(str_t str, char *fmt, ...)
{
	size_t res, new_res;
	va_list ap, ap_copy;

	va_start(ap, fmt);
	va_copy(ap_copy, ap);
	buffer_reset(str);
	buffer_advance(str, STR_SNPRINTF_ADVANCE);
	res = vsnprintf(buffer_ptr(str), STR_SNPRINTF_ADVANCE, fmt, ap);
	if (res < 0)
		error(1, "str_snprintf: vsnprintf res=%d", (int)res);
	if (res >= STR_SNPRINTF_ADVANCE) {
		buffer_reset(str);
		buffer_advance(str, res+1); /* plus one zero char */
		new_res = vsnprintf(buffer_ptr(str), res+1, fmt, ap_copy);
		if (new_res < 0 || new_res != res)
			error(1, "str_snprintf: should not reach");
	} else
		buffer_trim(str, STR_SNPRINTF_ADVANCE-res);
	/* now reduce buffer size down one byte because we have '\0' there */
	buffer_trim(str, 1);
	va_end(ap);
	return buffer_ptr(str);
}

char *str_drop(str_t str, size_t n)
{
	buffer_consume(str, n);
	return buffer_ptr(str);
}

char *str_trim(str_t str, size_t n)
{
	buffer_trim(str, n);
	buffer_zero(str);
	return buffer_ptr(str);
}

void str_reset(str_t str)
{
	buffer_reset(str);
	buffer_zero(str);
}

size_t str_len(str_t str)
{
	return buffer_size(str);
}
