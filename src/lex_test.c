#include <stdio.h>

#include "id_table.h"
#include "lex.h"


// допиши меня БЛЕАТЬ!!!!

extern struct lex_item lex_item;

/*typedef enum {
	} tok_t;
*/
char *tokens[] = {
	"TOK_KEYWORD",
	"TOK_ID",
	"TOK_VAR",

	"TOK_AS",
	"TOK_PLUS_AS",
	"TOK_MINUS_AS",
	"TOK_MUL_AS",
	"TOK_DIV_AS",
	"TOK_B_AND_AS",
	"TOK_B_XOR_AS",
	"TOK_B_OR_AS",
	"TOK_SHL_AS",
	"TOK_SHR_AS",
	"TOK_NOT",

	"TOK_EQ",
	"TOK_NEQ",
	"TOK_GR",
	"TOK_LO",
	"TOK_GE",
	"TOK_LE",
	"TOK_L_AND",
	"TOK_L_OR",

	"TOK_B_AND",
	"TOK_B_XOR",
	"TOK_B_OR",

	"TOK_LPAR",
	"TOK_RPAR",
	"TOK_LBRACE",
	"TOK_RBRACE",
	"TOK_LBRACKET",
	"TOK_RBRACKET",
	"TOK_COMMA",
	"TOK_SEMICOLON",
	"TOK_SHL",
	"TOK_SHR",

	"TOK_PLUS",
	"TOK_MINUS",
	"TOK_MUL",
	"TOK_DIV",

	"TOK_EOL",

	"TOK_DEF",
	"TOK_IF",
	"TOK_ELSE",
	"TOK_FOR",
	"TOK_DO",
	"TOK_WHILE",
	"TOK_CONTINUE",
	"TOK_BREAK",
	"TOK_RETURN",
	"TOK_IMPORT",

	"TOK_EOF",
	"TOK_UNKNOWN",
};

int
main()
{
	int res;

	set_input(stdin);
	id_table_init();

	do {
		res = get_next_token();
		printf("get token: %s ", tokens[res]);
		switch(res) {
		case TOK_ID:
			printf("name %s ", lex_item.name);
			break;
		case TOK_VAR:
			printf("var %p ", lex_item.var);
			break;
		}
		printf("\n");
	} while (/*res != TOK_EOL && */res != TOK_UNKNOWN);

	id_table_destroy();

	return 0;
}

