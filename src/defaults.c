#include <string.h>

int
default_hash_compare(const void *a, const void *b)
{
	return strcmp((char*)a, (char*)b);
}

unsigned long
default_hash_cb(const void *data)
{
	int i, mult, res;
	char *s;

	mult = 31;
	res = 0;
	s = (char*)data;

	for (i = 0; i < strlen(s); i++)
		res = res * mult + s[i];
	return res;
}

