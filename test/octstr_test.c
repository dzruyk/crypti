#include <stdio.h>

#include "octstr.h"

void
octstr_print(octstr_t *octstr)
{
	int i;
	unsigned char *ptr;

	ptr = octstr_ptr(octstr);

	for (i = 0; i < octstr_len(octstr); i++) {
		printf("\\x%2.2X", *ptr++ & 0xff);
	}
	printf("\n");
}

int
main()
{
	octstr_t a, b, c;

	octstr_initv(&a, &b, &c, NULL);
	
	octstr_append(&a, "hello \n");
	octstr_append(&b, "from the\n");
	octstr_append(&c, "side");

	octstr_append_octstr(&a, &b);
	octstr_append_octstr(&a, &c);

	printf("result string is = \n");
	
	octstr_print(&a);

	printf("expected\n"
	    "\\x68\\x65\\x6C\\x6F\\x20"
	    "\\x0A\\x66\\x72\\x6F\\x6D"
	    "\\x20\\x74\\x68\\x65\\x0A"
	    "\\x73\\x69\\x64\\x65");

	printf("====\n"
	    "bitwise tests:\n");

	octstr_reset(&a);
	octstr_reset(&b);
	
	octstr_append_n(&a, "\xA5\x5A", 2);
	octstr_append_n(&b, "\x5A\xA5", 2);

	octstr_or(&c, &a, &b);
	printf("octstr_or 0xA55A | 0x5AA5 = ");
	octstr_print(&c);

	octstr_and(&c, &a, &b);
	printf("0xA55A & 0x5AA5 = ");
	octstr_print(&c);

	octstr_xor(&c, &a, &b);
	printf("0xA55A ^ 0x5AA5 = ");
	octstr_print(&c);

	octstr_clearv(&a, &b, &c, NULL);

	return 0;
}

