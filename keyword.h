#ifndef __KEY_H__
#define __KEY_H__

typedef enum {
	KEY_DEF,		//function definition
	KEY_IF,
	KEY_ELSE,
	KEY_FOR,
	KEY_DO,
	KEY_WHILE,
	KEY_RETURN,
	KEY_UNKNOWN,
} keyword_t;

void keyword_table_init();

keyword_t keyword_table_lookup(char *name);

void keyword_table_destroy();

#endif

