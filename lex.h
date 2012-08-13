#ifndef __LEX_H__
#define __LEX_H__

#include "common.h"
#include "id_table.h"
#include "macros.h"

typedef enum {
	TOK_KEYWORD,
	TOK_ID,
	TOK_NUM,
	
	TOK_AS,
	TOK_NOT,
	
	TOK_EQ,
	TOK_NEQ,
	TOK_GR,
	TOK_LO,
	TOK_GE,
	TOK_LE,
	TOK_L_AND,
	TOK_L_OR,
	
	TOK_B_AND,
	TOK_B_OR,

	TOK_LPAR,
	TOK_RPAR,
	TOK_LBRACE,	// {
	TOK_RBRACE,
	TOK_LBRACKET,	// [
	TOK_RBRACKET,
	TOK_COMMA,
	TOK_SEMICOLON,	// ;
	
	TOK_PLUS,
	TOK_MINUS,
	TOK_MUL,
	TOK_DIV,
	
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

	TOK_EOF,
	TOK_UNKNOWN,
} tok_t;

struct lex_item {
	tok_t id;
	union {
		int num;
		char *name;
		int op;
	};
};

struct lex_item lex_item;

/* 
 * Get next token from stdin and fills
 * lex_item with token attributes
 */

tok_t get_next_token();

#endif

