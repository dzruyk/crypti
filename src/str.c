#include <assert.h>
#include <string.h>

#include "common.h"
#include "str.h"

str_t *
str_new(void *ptr, int len)
{
	assert(ptr != NULL && len >= 0);
	str_t *tmp;


	return tmp;
}

str_t *
str_new_clean(int len)
{
	assert(len >= 0);
	str_t *tmp;

	tmp = xmalloc(sizeof(*tmp));
	memset(tmp, 0, sizeof(*tmp));

	if (len > 0)
		tmp->buff = xmalloc(len);

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
	res = str_new_clean(left->len + right->len);
	//WRITEME
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

	if (first + len > str->len)
		len = str->len - first;
	
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
		printf("\\0x%x", str->buff[i] & 0xff);

}

void
str_del(str_t *str)
{
	assert (str != NULL);

	if (str->len > 0)
		ufree(str->buff);
	
	ufree(str);
}


