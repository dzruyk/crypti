#include <assert.h>

#include "common.h"
#include "str.h"


str_t *
str_new(int len)
{
	assert(len >= 0);
	str_t *tmp;

	tmp = xmalloc(sizeof(*tmp));
	memset(tmp, 0, sizeof(*tmp))

	if (len > 0)
		tmp->ptr = xmalloc(len);

	return tmp;
}

int 
str_len(str_t *str)
{
	assert(str != NULL);

	return str->len;
}

str_t *
str_resize(str_t *str, int sz)
{
	assert(str != NULL && sz >= 0);

}

str_t *
str_concat(str_t *left, str_t *right)
{

}

str_t *str_dup(str_t *str)
{

}

str_t *str_sub(str_t *str)
{

}

void str_print(str_t *str)
{

}

void str_del(str_t *str)
{
	assert (str != NULL);

	if (str->len > 0)
		ufree(str->ptr);
	
	ufree(str);
}


