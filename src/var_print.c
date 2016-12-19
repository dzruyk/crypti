#include <stdio.h>
#include <stdint.h>

#include "log.h"
#include "str.h"
#include "octstr.h"
#include "mpl.h"
#include "variable.h"
#include "var_print.h"

#define BUF_SZ 1024

int
str_append_int(str_t *dst, struct variable *src)
{
	mpl_int *tmp;
	//FIXME: fixed bufsz?
	char buf[BUF_SZ];
	int ret;

	tmp = var_cast_to_bignum(src);
	ret = mpl_to_str(tmp, buf, 10, BUF_SZ);
	if (ret != MPL_OK) {
		DEBUG(LOG_DEFAULT, "mpl_to_str convertion error\n");
		return -1;
	}
	str_append(dst, buf);
	return 0;
}

int
str_append_hex(str_t *dst, struct variable *src)
{
	octstr_t *tmp;
	char buf[5];
	unsigned char *s;
	int i, n;

	tmp = var_cast_to_octstr(src);
	n = octstr_len(tmp);
	s = octstr_ptr(tmp);

	for (i = 0; i < n; i++) {
		snprintf(buf, sizeof(buf), "\\x%2.2X", s[i]);
		str_append(dst, buf);
	}

	return 0;
}

int
str_append_s(str_t *dst, struct variable *src)
{
	str_t *tmp;
	tmp = var_cast_to_str(src);
	str_append_str(dst, tmp);

	return 0;
}

int
var_str_print_formatted(struct variable *dst, struct variable *fmt, struct variable **args, int nargs)
{
	str_t *str;
	str_t *d;
	char *s;

	d = var_str_ptr(dst);
	str_reset(d);

	str = var_cast_to_str(fmt);
	s = str_ptr(str);

	for (; *s != '\0'; s++) {
		if (*s == '\\') {
			s++;
			switch (*s) {
			case 't':
				str_putc(d, '\t');
				break;
			case 'n':
				str_putc(d, '\n');
				break;
			case 'r':
				str_putc(d, '\r');
				break;
			default:
				str_putc(d, *s);
				break;
			}
		} else if (*s == '%') {
			if (nargs <= 0) {
				DEBUG(LOG_VERBOSE, "to few arguments");
				str_putc(d, *s);
				continue;
			}
			s++;
			switch(*s) {
			case 'd':
				str_append_int(d, *args);
				break;
			case 'x':
				str_append_hex(d, *args);
				break;
			case 's':
				str_append_s(d, *args);
				break;
			case '%':
				str_putc(d, '%');
				break;
			default:
				DEBUG(LOG_DEFAULT, "unknown qualifier\n");
				str_putc(d, *s);
				break;
			}
			args++;
			nargs--;
		} else {
			str_putc(d, *s);
		}
	}

	if (nargs != 0)
		DEBUG(LOG_DEFAULT, "to many arguments\n");

	var_force_type(dst, VAR_STRING);

	return 0;
}


int
var_print_formatted(struct variable *fmt, struct variable **args, int nargs)
{
	struct variable var;
	int n;

	var_init(&var);

	n = var_str_print_formatted(&var, fmt, args, nargs);
	if (n == 0) {
		str_t *str;
		char *s;

		str = var_str_ptr(&var);
		s = str_ptr(str);
		printf("%s", s);
	} else {
		DEBUG(LOG_DEFAULT, "print formatted err\n");
	}

	var_clear(&var);

	return n;
}

