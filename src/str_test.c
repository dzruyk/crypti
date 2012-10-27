#include <stdio.h>

#include "str.h"

int
main()
{
	str_t a, b, c;

	str_initv(&a, &b, &c, NULL);
	
	str_append(&a, "hello \n");
	str_append(&b, "from the\n");
	str_append(&c, "side");

	str_append_str(&a, &b);
	str_append_str(&a, &c);

	printf("result string is = %s\n", str_ptr(&a));
	
	//print_str
	str_clearv(&a, &b, &c, NULL);

	return 0;
}

