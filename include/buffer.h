/*
 * buffer.h - buffer abstraction
 */

#ifndef BUFFERH_
#define BUFFERH_

struct buffer;

struct buffer *buffer_new();
struct buffer *buffer_create_static(void *buffer_ptr, size_t buffer_size);
struct buffer *buffer_create_static_full(void *buffer_ptr, size_t buffer_size);
void buffer_reset(struct buffer *buffer);
void buffer_advance(struct buffer *buffer, size_t size);
void buffer_free(struct buffer *buf);
inline void *buffer_ptr(struct buffer *buf);
inline size_t buffer_size(struct buffer *buffer);
void buffer_trim(struct buffer *buffer, size_t size);
void buffer_consume(struct buffer *buf, size_t size);
void buffer_putc(struct buffer *buffer, char c);
void buffer_zero(struct buffer *buffer);
void buffer_copy(struct buffer *dst, struct buffer *src, size_t size);
struct buffer *buffer_dup(struct buffer *buffer);
void buffer_peek(struct buffer *buffer, void *dst, size_t size, size_t offset);
void buffer_poke(struct buffer *buffer, const void *src, size_t size, size_t offset);

void buffer_put(struct buffer *buf, const void *src, size_t size);
void buffer_get(struct buffer *buffer, void *dst, size_t size);

void buffer_put_u8(struct buffer *buffer, uint8_t datum);
inline void buffer_put_u16(struct buffer *buffer, uint16_t datum);
inline void buffer_put_u32(struct buffer *buffer, uint32_t datum);

uint8_t buffer_get_u8(struct buffer *buffer);
inline uint16_t buffer_get_u16(struct buffer *buffer);
inline uint32_t buffer_get_u32(struct buffer *buffer);

uint16_t buffer_get_u16_le(struct buffer *buffer);
uint16_t buffer_get_u16_be(struct buffer *buffer);
uint32_t buffer_get_u32_le(struct buffer *buffer);
uint32_t buffer_get_u32_be(struct buffer *buffer);
void buffer_put_u16_le(struct buffer *buffer, uint16_t datum);
void buffer_put_u16_be(struct buffer *buffer, uint16_t datum);
void buffer_put_u32_le(struct buffer *buffer, uint32_t datum);
void buffer_put_u32_be(struct buffer *buffer, uint32_t datum);

void buffer_put_u8_at(struct buffer *buffer, uint8_t datum, size_t offset);
inline void buffer_put_u16_at(struct buffer *buffer, uint16_t datum, size_t offset);
inline void buffer_put_u32_at(struct buffer *buffer, uint32_t datum, size_t offset);

uint8_t buffer_get_u8_at(struct buffer *buffer, size_t offset);
inline uint16_t buffer_get_u16_at(struct buffer *buffer, size_t offset);
inline uint32_t buffer_get_u32_at(struct buffer *buffer, size_t offset);

uint16_t buffer_get_u16_le_at(struct buffer *buffer, size_t offset);
uint16_t buffer_get_u16_be_at(struct buffer *buffer, size_t offset);
uint32_t buffer_get_u32_le_at(struct buffer *buffer, size_t offset);
uint32_t buffer_get_u32_be_at(struct buffer *buffer, size_t offset);
void buffer_put_u16_le_at(struct buffer *buffer, uint16_t datum, size_t offset);
void buffer_put_u16_be_at(struct buffer *buffer, uint16_t datum, size_t offset);
void buffer_put_u32_le_at(struct buffer *buffer, uint32_t datum, size_t offset);
void buffer_put_u32_be_at(struct buffer *buffer, uint32_t datum, size_t offset);

float buffer_get_float_le_at(struct buffer *buffer, size_t offset);
float buffer_get_float_be_at(struct buffer *buffer, size_t offset);

inline void buffer_put_float_at(struct buffer *buffer, float datum, size_t offset);
inline float buffer_get_float_at(struct buffer *buffer, size_t offset);

void buffer_printf_append(struct buffer *buf, char *fmt, ...);
void buffer_put_string(struct buffer *buffer, const char *str);

/* Read and write wrappers using buffers. */
int buffer_read_fd(int fd, struct buffer *buffer, size_t size);
int buffer_write_fd(int fd, struct buffer *buffer);

#endif /*BUFFERH_*/
