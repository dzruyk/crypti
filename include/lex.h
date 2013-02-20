#ifndef __LEX_H__
#define __LEX_H__

#include "common.h"
#include "id_table.h"
#include "macros.h"

typedef enum {
	TOK_KEYWORD,
	TOK_ID,
	TOK_VAR,
	
	TOK_AS,
	TOK_PLUS_AS,
	TOK_MINUS_AS,
	TOK_MUL_AS,
	TOK_DIV_AS,
	TOK_POW_AS,
	TOK_B_AND_AS,
	TOK_B_XOR_AS,
	TOK_B_OR_AS,
	TOK_SHL_AS,	// <<=
	TOK_SHR_AS,	// >>=

	TOK_NOT,	// !
	
	TOK_EQ,
	TOK_NEQ,
	TOK_GR,
	TOK_LO,
	TOK_GE,
	TOK_LE,
	TOK_L_AND,
	TOK_L_OR,
	
	TOK_B_AND,
	TOK_B_XOR,
	TOK_B_OR,

	TOK_LPAR,
	TOK_RPAR,
	TOK_LBRACE,	// {
	TOK_RBRACE,	// }
	TOK_LBRACKET,	// [
	TOK_RBRACKET,	// ]
	TOK_COMMA,	// ,
	TOK_SEMICOLON,	// ;
	TOK_SHL,	// <<
	TOK_SHR,	// >>
	
	TOK_PLUS,
	TOK_MINUS,
	TOK_MUL,
	TOK_DIV,
	TOK_POW,	// **
	TOK_HASH,	//#
	
	TOK_EOL,

	/* keyword tokens */
	TOK_DEF,
	TOK_IF,
	TOK_ELSE,
	TOK_FOR,
	TOK_DO,
	TOK_WHILE,
	TOK_CONTINUE,
	TOK_BREAK,
	TOK_RETURN,
	TOK_IMPORT,
	TOK_DEL,

	/* specific tokens */

	TOK_EOF,
	TOK_UNKNOWN,
} tok_t;

struct lex_item {
	tok_t id;
	union {
		char *name;
		struct variable *var;
	};
};

struct lex_item lex_item;

/* 
 * Get next token from stdin and fills
 * lex_item with token attributes.
 */

tok_t get_next_token();

/* 
 * Set fp as current input file.
 */
void set_input(FILE *fp);

/*
 * Get current input file.
 */
FILE *get_input();

#endif

