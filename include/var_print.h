#ifndef __VAR_PRINT_H__
#define __VAR_PRINT_H__

int var_str_print_formatted(struct variable *dst, struct variable *fmt, struct variable **args, int nargs);

int var_print_formatted(struct variable *fmt, struct variable **args, int nargs);

#endif
