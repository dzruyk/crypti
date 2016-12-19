#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <endian.h>
#include <stdint.h>
#include <err.h>

#include "common.h"
#include "macros.h"

/*
 * TODO: change buffer_advance() behaviour, remove memset from it.
 */

#define BUFFER_INIT_SIZE        (4 * 1024)
/*#define BUFFER_INIT_SIZE	4*/
#define BUFFER_SPACE(b)         ((b)->data_size - (b)->head)
#define BUFFER_UNUSED(b)        ((b)->tail)
#define BUFFER_SIZE(b)          ((b)->head - (b)->tail)

#define BUFFER_CHECK(b) \
do { \
  if ((b) == NULL || \
      (b)->data == NULL || \
      (b)->data_size == 0 || \
      (b)->head > (b)->data_size || \
      (b)->tail > (b)->head) \
         error(1, "buffer: %p sanity check failed", (b)); \
} while (0)

struct buffer {
	size_t	head;
	size_t	tail;
	void	*data;
	size_t	data_size;
	int 	static_flag;
};

struct buffer *buffer_new()
{
	struct buffer *buffer;

	buffer = xmalloc(sizeof(struct buffer));

	buffer->data = xmalloc(BUFFER_INIT_SIZE);
	buffer->head = 0;
	buffer->tail = 0;
	buffer->data_size = BUFFER_INIT_SIZE;
	buffer->static_flag = 0;
	return buffer;
}

struct buffer *buffer_create_static(void *data, size_t buffer_size)
{
	struct buffer *buffer;

	if (data == NULL)
		error(1, "buffer: get invalid argument NULL!");

	if (buffer_size > SSIZE_MAX || buffer_size == 0)
		error(1, "buffer: static %lu", (unsigned long)buffer_size);

	buffer = xmalloc(sizeof(struct buffer));
	buffer->head = 0;
	buffer->tail = 0;
	buffer->data = data;
	buffer->data_size = buffer_size;
	buffer->static_flag = 1;

	return buffer;
}

struct buffer *buffer_create_static_full(void *data, size_t buffer_size)
{
	struct buffer *buffer;

	if (data == NULL)
		error(1, "buffer: get invalid argument NULL!");

	if (buffer_size > SSIZE_MAX || buffer_size == 0)
		error(1, "buffer: static %lu", (unsigned long)buffer_size);

	buffer = xmalloc(sizeof(struct buffer));
	buffer->head = buffer_size;
	buffer->tail = 0;
	buffer->data = data;
	buffer->data_size = buffer_size;
	buffer->static_flag = 1;

	return buffer;
}

void buffer_reset(struct buffer *buffer)
{
	BUFFER_CHECK(buffer);

	/* clean data and set head and tail to init state */
	memset(buffer->data, 0, buffer->data_size);
	buffer->tail = 0;
	buffer->head = 0;
}

void buffer_free(struct buffer *buffer)
{
	BUFFER_CHECK(buffer);

	if (!buffer->static_flag)
		ufree(buffer->data);

	ufree(buffer);
}

inline void *buffer_ptr(struct buffer *buffer)
{
	return buffer->data + buffer->tail;
}

inline size_t buffer_size(struct buffer *buffer)
{
	return BUFFER_SIZE(buffer);
}

void buffer_trim(struct buffer *buffer, size_t size)
{
	BUFFER_CHECK(buffer);

	if (size == 0) {
		warning("buffer_trim: trim 0 bytes");
		return;
	}

	if (size > SSIZE_MAX || size > BUFFER_SIZE(buffer))
		error(1, "buffer: trim %lu size %lu bytes",
				(unsigned long)size, (unsigned long)BUFFER_SIZE(buffer));

	memset(buffer->data + buffer->head - size, 0, size);
	buffer->head -= size;

	return;
}

void buffer_consume(struct buffer *buffer, size_t size)
{
	BUFFER_CHECK(buffer);

	if (size == 0) {
		warning("buffer: consume 0 bytes");
		return;
	}

	if (size > SSIZE_MAX || size > BUFFER_SIZE(buffer))
		error(1, "buffer: consume %lu buffer size %lu bytes",
				(unsigned long)size, (unsigned long)BUFFER_SIZE(buffer));

	buffer->tail += size;
}

static void buffer_ensure(struct buffer *buffer, size_t size)
{
	size_t new_size;

	if (BUFFER_SPACE(buffer) < size) {
		if (BUFFER_UNUSED(buffer) + BUFFER_SPACE(buffer) >= size) {
			memmove(buffer->data, buffer->data + buffer->tail, BUFFER_SIZE(buffer));
			buffer->head -= buffer->tail;
			buffer->tail = 0;
		} else {
			if (buffer->static_flag)
				error(1, "buffer: ensure static buffer oversize");
			new_size = buffer->data_size;
			do {
				new_size <<= 1;
				if (new_size > SSIZE_MAX)
					error(1, "buffer: ensure buffer oversize");
			} while (new_size - buffer->head < size);
			buffer->data = xrealloc(buffer->data, new_size);
			buffer->data_size = new_size;
		}
	}
}

void buffer_advance(struct buffer *buffer, size_t size)
{
	BUFFER_CHECK(buffer);

	if (size > SSIZE_MAX)
		error(1, "buffer: advance size %lu", (unsigned long) size);

	buffer_ensure(buffer, size);
	memset(buffer->data + buffer->head, 0, size);
	buffer->head += size;
}

void buffer_printf_append(struct buffer *buffer, char *fmt, ...)
{
	va_list ap, ap_save;
	int res, new_res;
	char *p;
	size_t head;

#define VSNPRINTF_TRY   32

	va_start(ap, fmt);
	va_copy(ap_save, ap);

	head = buffer->head;
	buffer_advance(buffer, VSNPRINTF_TRY);
	p = buffer->data + head;

	res = vsnprintf(p, VSNPRINTF_TRY, fmt, ap);
	if (res < 0)
		errx(1, "vsnprint res=%d", res);
	if (res >= VSNPRINTF_TRY) {
		buffer_trim(buffer, VSNPRINTF_TRY);
		buffer_advance(buffer, res+1);
		p = buffer->data + head;
		new_res = vsnprintf(p, res+1, fmt, ap_save);
		if (new_res < 0 || new_res != res)
			error(1, "buffer: should not reach");
	} else {
		buffer_trim(buffer, VSNPRINTF_TRY-res-1);
	}

	buffer_trim(buffer, 1);
	va_end(ap);
}

void buffer_put_string(struct buffer *buffer, const char *str)
{
	char *ptr;
	size_t len;

	BUFFER_CHECK(buffer);

	if (str == NULL)
		error(1, "buffer: invalid string argument NULL!");

	len = strlen(str);
	buffer_ensure(buffer, len+1);

	ptr = (char *)buffer->data + buffer->head;
	memcpy(ptr, str, len+1);
	buffer->head += len+1;
}

void buffer_putc(struct buffer *buffer, char c)
{
	char *ptr;

	BUFFER_CHECK(buffer);

	buffer_ensure(buffer, sizeof(char));
	ptr = (char*) buffer->data + buffer->head;
	ptr[0] = c;
	buffer->head += sizeof(c);
}

void buffer_zero(struct buffer *buffer)
{
	char *ptr;

	BUFFER_CHECK(buffer);

	buffer_ensure(buffer, sizeof(char));
	ptr = (char*) buffer->data + buffer->head;
	ptr[0] = '\0';
}

void buffer_put(struct buffer *buffer, const void *src, size_t size)
{
	BUFFER_CHECK(buffer);

	if (src == NULL)
		error(1, "buffer: put invalid argument NULL!");

	if (size > SSIZE_MAX)
		error(1, "buffer: put %lu bytes", (unsigned long)size);

	buffer_ensure(buffer, size);

	memcpy(buffer->data + buffer->head, src, size);
	buffer->head += size;
}

void buffer_put_u16_le(struct buffer *buffer, uint16_t datum)
{
	unsigned char *data;

	buffer_ensure(buffer, sizeof(datum));
	data = buffer->data + buffer->head;

	data[0] = datum & 0xff;
	data[1] = (datum >> 8) & 0xff;

	buffer->head += sizeof(datum);
}

void buffer_put_u16_be(struct buffer *buffer, uint16_t datum)
{
	unsigned char *data;

	buffer_ensure(buffer, sizeof(datum));
	data = buffer->data + buffer->head;

	data[1] = datum & 0xff;
	data[0] = (datum >> 8) & 0xff;

	buffer->head += sizeof(datum);
}

void buffer_put_u32_le(struct buffer *buffer, uint32_t datum)
{
	unsigned char *data;

	buffer_ensure(buffer, sizeof(datum));
	data = buffer->data + buffer->head;

	data[0] = datum & 0xff;
	data[1] = (datum >> 8) & 0xff;
	data[2] = (datum >> 16) & 0xff;
	data[3] = (datum >> 24) & 0xff;

	buffer->head += sizeof(datum);
}

void buffer_put_u32_be(struct buffer *buffer, uint32_t datum)
{
	unsigned char *data;

	buffer_ensure(buffer, sizeof(datum));
	data = buffer->data + buffer->head;

	data[0] = (datum >> 24) & 0xff;
	data[1] = (datum >> 16) & 0xff;
	data[2] = (datum >> 8) & 0xff;
	data[3] = datum & 0xff;

	buffer->head += sizeof(datum);
}

void buffer_put_u8(struct buffer *buffer, uint8_t datum)
{
	unsigned char *data;

	BUFFER_CHECK(buffer);
	buffer_ensure(buffer, sizeof(datum));
	data = buffer->data + buffer->head;
	data[0] = datum & 0xff;
	buffer->head += sizeof(datum);
}

inline void buffer_put_u16(struct buffer *buffer, uint16_t datum)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	buffer_put_u16_le(buffer, datum);
#elif BYTE_ORDER == BIG_ENDIAN
	buffer_put_u16_be(buffer, datum);
#else
  #error byte order not defined!
#endif
}

inline void buffer_put_u32(struct buffer *buffer, uint32_t datum)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	buffer_put_u32_le(buffer, datum);
#elif BYTE_ORDER == BIG_ENDIAN
	buffer_put_u32_be(buffer, datum);
#else
  #error byte order not defined!
#endif
}

void buffer_get(struct buffer *buffer, void *dst, size_t size)
{
	BUFFER_CHECK(buffer);

	if (dst == NULL)
		error(1, "buffer: get invalid argument NULL!");

	if (size == 0)
		warning("buffer: get size 0");

	if (size > SSIZE_MAX || size > BUFFER_SIZE(buffer))
		error(1, "buffer: get %lu bytes buffer size %lu bytes",
			(unsigned long)size, (unsigned long)BUFFER_SIZE(buffer));

	memcpy(dst, buffer->data + buffer->tail, size);

	buffer->tail += size;
}

uint16_t buffer_get_u16_le(struct buffer *buffer)
{
	uint16_t val;
	unsigned char *data;

	BUFFER_CHECK(buffer);
	data = buffer->data + buffer->tail;

	val  = data[0];
	val |= data[1] << 8;

	buffer->tail += sizeof(val);
	return val;
}

uint16_t buffer_get_u16_be(struct buffer *buffer)
{
	uint16_t val;
	unsigned char *data;

	BUFFER_CHECK(buffer);
	data = buffer->data + buffer->tail;

	val  = data[1];
	val |= data[0] << 8;

	buffer->tail += sizeof(val);
	return val;
}

uint32_t buffer_get_u32_le(struct buffer *buffer)
{
	uint32_t val;
	unsigned char *data;

	BUFFER_CHECK(buffer);
	data = buffer->data + buffer->tail;

	val  = data[0];
	val |= data[1] << 8;
	val |= data[2] << 16;
	val |= data[3] << 24;

	buffer->tail += sizeof(val);
	return val;
}

uint32_t buffer_get_u32_be(struct buffer *buffer)
{
	uint32_t val;
	unsigned char *data;

	BUFFER_CHECK(buffer);
	data = buffer->data + buffer->tail;

	val  = data[3];
	val |= data[2] << 8;
	val |= data[1] << 16;
	val |= data[0] << 24;

	buffer->tail += sizeof(val);
	return val;
}

uint8_t buffer_get_u8(struct buffer *buffer)
{
	uint8_t val;
	unsigned char *data;

	BUFFER_CHECK(buffer);
	data = buffer->data + buffer->tail;
	val = data[0] & 0xFF;
	buffer->tail += sizeof(val);
	return  val;
}

inline uint16_t buffer_get_u16(struct buffer *buffer)
{
	uint16_t val;

#if BYTE_ORDER == LITTLE_ENDIAN
	val = buffer_get_u16_le(buffer);
#elif BYTE_ORDER == BIG_ENDIAN
	val = buffer_get_u16_be(buffer);
#else
  #error byte order not defined!
#endif
	return val;
}

inline uint32_t buffer_get_u32(struct buffer *buffer)
{
	uint32_t val;

#if BYTE_ORDER == LITTLE_ENDIAN
	val = buffer_get_u32_le(buffer);
#elif BYTE_ORDER == BIG_ENDIAN
	val = buffer_get_u32_be(buffer);
#else
  #error byte order not defined!
#endif
	return val;
}

static inline void _buffer_append(struct buffer *dst, struct buffer *src, size_t size)
{
	buffer_ensure(dst, size);
	memcpy(dst->data + dst->head, src->data + src->tail, size);
	dst->head += size;
}

void buffer_append(struct buffer *dst, struct buffer *src, size_t size)
{
	BUFFER_CHECK(dst);
	BUFFER_CHECK(src);

	if (size > SSIZE_MAX)
		error(1, "buffer: get %lu bytes", (unsigned long) size);

	if (size > BUFFER_SIZE(src))
		error(1, "buffer: copy %lu bytes src buffer size %lu bytes",
				(unsigned long)size, (unsigned long)BUFFER_SIZE(src));

	_buffer_append(dst, src, size);
}

void buffer_copy(struct buffer *dst, struct buffer *src, size_t size)
{
	BUFFER_CHECK(dst);
	BUFFER_CHECK(src);

	buffer_reset(dst);
	_buffer_append(dst, src, size);
}

struct buffer *buffer_dup(struct buffer *buffer)
{
	struct buffer *new;

	BUFFER_CHECK(buffer);
	new = buffer_new();
	buffer_append(new, buffer, BUFFER_SIZE(buffer));

	return new;
}

uint8_t buffer_get_u8_at(struct buffer *buffer, size_t offset)
{
	uint8_t res;
	unsigned char *data;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_u8_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(res) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_u8_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;
	res = data[0];
	return res;
}

void buffer_put_u8_at(struct buffer *buffer, uint8_t datum, size_t offset)
{
	unsigned char *ptr;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_u8_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_u8_at: offset+datum beyound buffer");

	ptr = buffer->data + buffer->tail + offset;
	ptr[0] = datum;
}

uint16_t buffer_get_u16_le_at(struct buffer *buffer, size_t offset)
{
	uint16_t res;
	unsigned char *data;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_u16_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(res) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_u16_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;

	res  = data[0];
	res |= data[1] << 8;

	return res;
}

uint16_t buffer_get_u16_be_at(struct buffer *buffer, size_t offset)
{
	uint16_t res;
	unsigned char *data;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_u16_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(res) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_u16_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;

	res  = data[1];
	res |= data[0] << 8;

	return res;
}

void buffer_put_u16_le_at(struct buffer *buffer, uint16_t datum, size_t offset)
{
	unsigned char *ptr;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_u16_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_u16_at: offset+datum beyound buffer");

	ptr = buffer->data + buffer->tail + offset;

	ptr[0] = datum & 0xff;
	ptr[1] = (datum >> 8) & 0xff;
}

void buffer_put_u16_be_at(struct buffer *buffer, uint16_t datum, size_t offset)
{
	unsigned char *ptr;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_u16_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_u16_at: offset+datum beyound buffer");

	ptr = buffer->data + buffer->tail + offset;

	ptr[1] = datum & 0xff;
	ptr[0] = (datum >> 8) & 0xff;
}

uint32_t buffer_get_u32_le_at(struct buffer *buffer, size_t offset)
{
	uint32_t res;
	unsigned char *data;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_u32_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(res) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_u32_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;

	res  = data[0];
	res |= data[1] << 8;
	res |= data[2] << 16;
	res |= data[3] << 24;

	return res;
}

uint32_t buffer_get_u32_be_at(struct buffer *buffer, size_t offset)
{
	uint32_t res;
	unsigned char *data;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_u32_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(res) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_u32_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;

	res  = data[3];
	res |= data[2] << 8;
	res |= data[1] << 16;
	res |= data[0] << 24;

	return res;
}

void buffer_put_u32_le_at(struct buffer *buffer, uint32_t datum, size_t offset)
{
	unsigned char *ptr;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_u32_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_u32_at: offset+datum beyound buffer");

	ptr = buffer->data + buffer->tail + offset;

	ptr[0] = datum & 0xff;
	ptr[1] = (datum >> 8) & 0xff;
	ptr[2] = (datum >> 16) & 0xff;
	ptr[3] = (datum >> 24) & 0xff;

}

void buffer_put_u32_be_at(struct buffer *buffer, uint32_t datum, size_t offset)
{
	unsigned char *ptr;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_u32_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_u32_at: offset+datum beyound buffer");

	ptr = buffer->data + buffer->tail + offset;

	ptr[0] = (datum >> 24) & 0xff;
	ptr[1] = (datum >> 16) & 0xff;
	ptr[2] = (datum >> 8) & 0xff;
	ptr[3] = datum & 0xff;
}

inline void buffer_put_u16_at(struct buffer *buffer, uint16_t datum, size_t offset)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	buffer_put_u16_le_at(buffer, datum, offset);
#elif BYTE_ORDER == BIG_ENDIAN
	buffer_put_u16_be_at(buffer, datum, offset);
#else
  #error byte order not defined!
#endif
}

inline void buffer_put_u32_at(struct buffer *buffer, uint32_t datum, size_t offset)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	buffer_put_u32_le_at(buffer, datum, offset);
#elif BYTE_ORDER == BIG_ENDIAN
	buffer_put_u32_be_at(buffer, datum, offset);
#else
  #error byte order not defined!
#endif
}

inline uint16_t buffer_get_u16_at(struct buffer *buffer, size_t offset)
{
	uint16_t val;

#if BYTE_ORDER == LITTLE_ENDIAN
	val = buffer_get_u16_le_at(buffer, offset);
#elif BYTE_ORDER == BIG_ENDIAN
	val = buffer_get_u16_be_at(buffer, offset);
#else
  #error byte order not defined!
#endif
	return val;
}

inline uint32_t buffer_get_u32_at(struct buffer *buffer, size_t offset)
{
	uint32_t val;

#if BYTE_ORDER == LITTLE_ENDIAN
	val = buffer_get_u32_le_at(buffer, offset);
#elif BYTE_ORDER == BIG_ENDIAN
	val = buffer_get_u32_be_at(buffer, offset);
#else
  #error byte order not defined!
#endif
	return val;
}

float buffer_get_float_le_at(struct buffer *buffer, size_t offset)
{
	unsigned char *data;
	union {
		float res_float;
		uint32_t res_u32;
	} res;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_float_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(float) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_float_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;
	res.res_u32  = data[0];
	res.res_u32 |= data[1] << 8;
	res.res_u32 |= data[2] << 16;
	res.res_u32 |= data[3] << 24;
	return res.res_float;
}

float buffer_get_float_be_at(struct buffer *buffer, size_t offset)
{
	unsigned char *data;
	union {
		float res_float;
		uint32_t res_u32;
	} res;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_get_float_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(float) > BUFFER_SIZE(buffer))
		error(1, "buffer_get_float_at: offset beyound buffer");

	data = buffer->data + buffer->tail + offset;
	res.res_u32  = data[3];
	res.res_u32 |= data[2] << 8;
	res.res_u32 |= data[1] << 16;
	res.res_u32 |= data[0] << 24;
	return res.res_float;
}

inline float buffer_get_float_at(struct buffer *buffer, size_t offset)
{
	float val;

#if BYTE_ORDER == LITTLE_ENDIAN
	val = buffer_get_float_le_at(buffer, offset);
#elif BYTE_ORDER == BIG_ENDIAN
	val = buffer_get_float_be_at(buffer, offset);
#else
  #error byte order not defined!
#endif
	return val;
}

void buffer_put_float_le_at(struct buffer *buffer, float datum, size_t offset)
{
	unsigned char *ptr;
	union value {
		float float_val;
		uint32_t uint_val;
	} *p;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_float_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_float_at: offset+datum beyound buffer");

	p = (union value *) &datum;
	ptr = buffer->data + buffer->tail + offset;
	ptr[0] = p->uint_val & 0xff;
	ptr[1] = (p->uint_val >> 8) & 0xff;
	ptr[2] = (p->uint_val >> 16) & 0xff;
	ptr[3] = (p->uint_val >> 24) & 0xff;
}

void buffer_put_float_be_at(struct buffer *buffer, float datum, size_t offset)
{
	unsigned char *ptr;
	union value {
		float float_val;
		uint32_t uint_val;
	} *p;

	BUFFER_CHECK(buffer);

	if (offset > SSIZE_MAX)
		error(1, "buffer_put_float_at: offset %lu", (unsigned long) offset);

	if (offset + sizeof(datum) > BUFFER_SIZE(buffer))
		error(1, "buffer_put_float_at: offset+datum beyound buffer");

	p = (union value *) &datum;
	ptr = buffer->data + buffer->tail + offset;
	ptr[3] = p->uint_val & 0xff;
	ptr[2] = (p->uint_val >> 8) & 0xff;
	ptr[1] = (p->uint_val >> 16) & 0xff;
	ptr[0] = (p->uint_val >> 24) & 0xff;
}

inline void buffer_put_float_at(struct buffer *buffer, float datum, size_t offset)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	buffer_put_float_le_at(buffer, datum, offset);
#elif BYTE_ORDER == BIG_ENDIAN
	buffer_put_float_be_at(buffer, datum, offset);
#else
  #error byte order not defined!
#endif
}

void buffer_peek(struct buffer *buffer, void *dst, size_t size, size_t offset)
{
	BUFFER_CHECK(buffer);

	if (dst == NULL)
		error(1, "buffer: peek invalid argument NULL!");

	if (size == 0)
		warning("buffer: peek size 0");

	if (size > SSIZE_MAX || offset > SSIZE_MAX)
		error(1, "buffer: peek %lu bytes offset %lu bytes",
					(unsigned long)size, (unsigned long)offset);

	if (offset > BUFFER_SIZE(buffer) ||
	    size > BUFFER_SIZE(buffer) ||
	    offset + size > BUFFER_SIZE(buffer))
		error(1, "buffer: peek %lu bytes offset %lu bytes buffer size %lu bytes",
				(unsigned long) size, (unsigned long) offset,
				(unsigned long) BUFFER_SIZE(buffer));

	memcpy(dst, buffer->data + buffer->tail + offset, size);
}

void buffer_poke(struct buffer *buffer, const void *src, size_t size, size_t offset)
{
	BUFFER_CHECK(buffer);

	if (src == NULL)
		error(1, "buffer: poke invalid argument NULL!");

	if (size == 0)
		warning("buffer: poke size 0");

	if (size > SSIZE_MAX || offset > SSIZE_MAX)
		error(1, "buffer: poke %lu bytes offset %lu bytes",
					(unsigned long) size, (unsigned long) offset);

	if (offset > BUFFER_SIZE(buffer) ||
	    size > BUFFER_SIZE(buffer) ||
	    offset + size > BUFFER_SIZE(buffer))
		error(1, "buffer: poke %lu bytes offset %lu bytes buffer size %lu bytes",
				(unsigned long) size, (unsigned long) offset,
				(unsigned long) BUFFER_SIZE(buffer));

	memcpy(buffer->data + buffer->tail + offset, src, size);
}

#define BUFFER_READ_SIZE	4096

int buffer_read_fd(int fd, struct buffer *buffer, size_t size)
{
	int res, n;

	/* get size of data in fd in advance */
	res = ioctl(fd, FIONREAD, &n);

	if (res == -1 || n == 0)
		n = BUFFER_READ_SIZE;
	if (res > 0) {
		/* It is possible that we have
		 * huge amount of data in fd. Then
		 * don't waist resources and do partial
		 * read.
		 */
		if (n > buffer->data_size << 2)
			n = buffer->data_size << 2;
		else
			n = res;
	}
	/* size is only a hint on required buffer size */
	size = size > n ? size : n;
	buffer_ensure(buffer, size);

	res = read(fd, buffer->data + buffer->head, size);

	if (res == 0)
		return 0;
	if (res == -1)
		return -1;
	buffer->head += res;
	return res;
}

int buffer_write_fd(int fd, struct buffer *buffer)
{
	int res;

	res = write(fd, buffer->data + buffer->tail, BUFFER_SIZE(buffer));

	if (res == 0)
		return 0;
	if (res == -1)
		return -1;

	buffer->tail += res;

	if (buffer->tail == buffer->head)
		buffer_reset(buffer);

	return res;
}

