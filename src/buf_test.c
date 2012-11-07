#include <stdio.h>
#include <stdint.h>

#include "buffer.h"

int
main()
{
	struct buffer *a, *b;

	a = buffer_new();
	b = buffer_new();
	
	buffer_putc(a, 'a');
	buffer_putc(a, 'g');
	buffer_copy(b, a, buffer_size(a));
	buffer_copy(b, a, buffer_size(a));

	buffer_zero(b);
	
	printf("buf sz = %d\n", buffer_size(b));
	return 0;
}
