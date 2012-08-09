#ifndef __KEY_H__
#define __KEY_H__

#include "lex.h"

void keyword_table_init();

tok_t keyword_table_lookup(char *name);

void keyword_table_destroy();

#endif

