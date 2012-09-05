#include <string.h>

#include "common.h"

inline void
print_usage(char *pname)
{
	printf("USAGE:%s\n", pname);
}

inline void *
xmalloc(size_t sz)
{	
	void *tmp;
	if ((tmp = malloc(sz)) == NULL)
		print_warn_and_die("malloc_err");
	return tmp;
}

inline void *
realloc_or_die(void *ptr, size_t sz)
{
	void *tmp;
	if ((tmp = realloc(ptr, sz)) == NULL)
		print_warn_and_die("malloc_err");
	return tmp;
}

inline void *
strdup_or_die(char *str)
{
	char *dup;

	dup = strdup(str);
	if (dup == NULL)
		print_warn_and_die("malloc_err");
	
	return dup;
}

inline void
ufree(void *data)
{
	if (data != NULL)
		free(data);
}


