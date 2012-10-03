#include <assert.h>
#include <string.h>

#include "common.h"
#include "str.h"

str_t *
str_new(void *ptr, int len)
{
	assert(ptr != NULL && len >= 0);

	str_t *tmp;
	char *src;
	int i;

	src = (char *) ptr;

	tmp = str_new_clean(len);

	for (i = 0; i < len; i++)
		tmp->buff[i] = src[i];
	
	return tmp;
}

str_t *
str_new_clean(int len)
{
	assert(len >= 0);
	str_t *tmp;

	tmp = xmalloc(sizeof(*tmp));
	memset(tmp, 0, sizeof(*tmp));

	if (len > 0) {
		tmp->buff = xmalloc(len);
		memset(tmp->buff, 0, len);
	}
	tmp->len = len;

	return tmp;
}

int 
str_len(str_t *str)
{
	assert(str != NULL);

	return str->len;
}

void
str_resize(str_t *str, int sz)
{
	assert(str != NULL && sz >= 0);
	if (str->len == sz)
		return;
	
	str->len = sz;
	str->buff = realloc_or_die(str->buff, sz);

	if (sz == 0)
		str->buff = NULL;
}

str_t *
str_concat(str_t *left, str_t *right)
{
	assert(left != NULL && right != NULL);
	
	str_t *res;
	unsigned char *dst;
	int i;

	res = str_new_clean(left->len + right->len);
	
	dst = res->buff;
	for (i = 0; i < left->len; i++)
		*dst++ = left->buff[i];
	
	for (i = 0; i < right->len; i++)
		*dst++ = right->buff[i];

	return res;
}

str_t *
str_dup(str_t *str)
{
	assert(str != NULL);

	return str_sub(str, 0, str->len);
}

str_t *
str_sub(str_t *str, int first, int len)
{
	assert(str != NULL && first >= 0);
	
	str_t *tmp;
	int i;

	if (first > str->len)
		return str_new_clean(0);

	if (first + len > str->len)
		len = str->len - first + 1;
	
	tmp = str_new_clean(len);

	//FIXME: rewrite me with ptrs.
	for (i = 0; i < len; i++)
		tmp->buff[first + i] = str->buff[first + i];
	
	return tmp;
}

void 
str_print(str_t *str)
{
	assert(str != NULL);

	int i;

	for (i = 0; i < str->len; i++)
		printf("%c", str->buff[i] & 0xff);

}

void
str_del(str_t *str)
{
	assert (str != NULL);

	ufree(str->buff);
	ufree(str);
}


