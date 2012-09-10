#include <stdio.h>

#include "id_table.h"
#include "lex.h"


// допиши меня БЛЕАТЬ!!!!

extern struct lex_item lex_item;

/*typedef enum {
	} tok_t;
*/
char *tokens[] = {"TOK_ID",
	"TOK_NUM",
	"TOK_AS",
	"TOK_RELOP",
	"TOK_LPAR",
	"TOK_RPAR",
	"TOK_PLUS",
	"TOK_MINUS",
	"TOK_MUL",
	"TOK_DIV",
	"TOK_EOL",
	"TOK_UNKNOWN"};

int
main()
{
	int res;

	id_table_init();

	do {
		res = get_next_token();
		printf("get token: %s ", tokens[res]);
		switch(res) {
		case TOK_ID:
			printf("%s ", lex_item.name);
			break;
		case TOK_NUM:
			printf("%d ", lex_item.num);
			break;
		}
		printf("\n");
	} while(/*res != TOK_EOL && */res != TOK_UNKNOWN);
	
	id_table_destroy();
	
	return 0;
}

