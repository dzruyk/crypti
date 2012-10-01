#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "keyword.h"
#include "lex.h"

static char peek = ' ';

/* current input file */
FILE *input;

static void
skip_comment()
{
	do
		peek = fgetc(input);
	while (peek != '\n');
}

tok_t
get_string()
{	
	char *s = NULL;
	char *tmp;
	int len, used;

	used = 0;
	len = 64;
	s = xmalloc(len);

	peek = fgetc(input);

	while (peek != EOF && peek != '"') {

		if (used >= len - 1) {
			len += 64;
			s = realloc_or_die(s, len);
		}

		s[used++] = peek;
		peek = fgetc(input);
	}

	if (peek != '"') {
		print_warn("uncomplited string");
		free(s);
		peek = ' ';
		return TOK_UNKNOWN;
	}

	s[used++] = '\0';

	if ((tmp = realloc(s, used)) == NULL)
		print_warn_and_die("realloc_err");
	s = tmp;

	lex_item.id = TOK_STRING;
	lex_item.str = s;

	peek = ' ';

	return TOK_STRING;
}

tok_t
get_next_token()
{

begin:

	for (; peek == ' ' || peek == '\t';)
		peek = fgetc(input);
	
	if (isdigit(peek)) {
		int num = 0;
		do {
			num = num * 10 + peek - '0';
			peek = fgetc(input);
		} while (isdigit(peek));
		
		lex_item.id = TOK_NUM;
		lex_item.num = num;
		return TOK_NUM;
	}

	if (isalpha(peek) || peek == '_') {
		tok_t kword;
		char *s = NULL;
		char *tmp;
		int used, len;
		
		len = used = 0;
		do {
			if (used >= len - 1) {
				len += 64;
				s = realloc_or_die(s, len);
			}

			s[used++] = peek;
			peek = fgetc(input);
		} while(isalnum(peek) || peek == '_');

		s[used++] = '\0';
		if ((tmp = realloc(s, used)) == NULL)
			print_warn_and_die("realloc_err");
		s = tmp;

		if ((kword = keyword_table_lookup(s)) != TOK_UNKNOWN) {
			free(s);
			lex_item.id = kword;
			return kword;
		}
		
		lex_item.id = TOK_ID;
		lex_item.name = s;
		
		return TOK_ID;
	}
	if (peek == '\"')
		return get_string();

	switch (peek) {
	case '=':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_EQ;
			peek = ' ';
			return TOK_EQ;
		}
		lex_item.id = TOK_AS;
		
		return TOK_AS;
	case '!':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_NEQ;
			
			return TOK_NEQ;
		}
		lex_item.id = TOK_NOT;

		return TOK_NOT;
	case '<':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_LE;
			
			return TOK_LE;
		} else if (peek == '<') {
			peek = fgetc(input);
			if (peek == '=') {
				peek = ' ';
				lex_item.id = TOK_SHL_AS;

				return TOK_SHL_AS;
			}
			lex_item.id = TOK_SHL;
			
			return TOK_SHL;
		}
		lex_item.id = TOK_LO;
		
		return TOK_LO;
	case '>':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_GE;
			
			return TOK_GE;
		} else if (peek == '>') {
			peek = fgetc(input);
			if (peek == '=') {
				peek = ' ';
				lex_item.id = TOK_SHR_AS;

				return TOK_SHR_AS;
			}
			lex_item.id = TOK_SHR;
			
			return TOK_SHR;
		}
		lex_item.id = TOK_GR;
		
		return TOK_GR;
	case '&':
		peek = fgetc(input);
		if (peek == '&') {
			peek = ' ';
			lex_item.id = TOK_L_AND;
			
			return TOK_L_AND;
		} else if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_B_AND_AS;

			return TOK_B_AND_AS;
		}
		lex_item.id = TOK_B_AND;
		
		return TOK_B_AND;
	case '|':
		peek = fgetc(input);
		if (peek == '|') {
			peek = ' ';
			lex_item.id = TOK_L_OR;
			
			return TOK_L_OR;
		} else if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_B_OR_AS;

			return TOK_B_OR_AS;
		}
		lex_item.id = TOK_B_OR;
		
		return TOK_B_OR;
	case '^':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_B_XOR_AS;

			return TOK_B_XOR_AS;
		}
		lex_item.id = TOK_B_XOR;

		return TOK_B_XOR;
	case '(':
		peek = ' ';
		lex_item.id = TOK_LPAR;
		
		return TOK_LPAR;
	case ')':
		peek = ' ';
		lex_item.id = TOK_RPAR;
		
		return TOK_RPAR;
	case '[':
		peek = ' ';
		lex_item.id = TOK_LBRACKET;
		
		return TOK_LBRACKET;
	case ']':
		peek = ' ';
		lex_item.id = TOK_RBRACKET;
		
		return TOK_RBRACKET;
	case '{':
		peek = ' ';
		lex_item.id = TOK_LBRACE;
		
		return TOK_LBRACE;
	case '}':
		peek = ' ';
		lex_item.id = TOK_RBRACE;
		
		return TOK_RBRACE;
	case ',':
		peek = ' ';
		lex_item.id = TOK_COMMA;

		return TOK_COMMA;
	case ';':
		peek = ' ';
		lex_item.id = TOK_SEMICOLON;

		return TOK_SEMICOLON;
	case '+':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_PLUS_AS;
			return TOK_PLUS_AS;
		}
		lex_item.id = TOK_PLUS;
		
		return TOK_PLUS;
	case '-':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_MINUS_AS;
			return TOK_MINUS_AS;
		}
		lex_item.id = TOK_MINUS;
		
		return TOK_MINUS;
	case '*':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_MUL_AS;
			return TOK_MUL_AS;
		}
		lex_item.id = TOK_MUL;
		
		return TOK_MUL;
	case '/':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_DIV_AS;
			return TOK_DIV_AS;
		}
		lex_item.id = TOK_DIV;
		
		return TOK_DIV;
	case '#':

		skip_comment();

		goto begin;
	case '\n':
		peek = ' ';
		lex_item.id = TOK_EOL;
		
		return TOK_EOL;
	case EOF:
		peek = ' ';
		lex_item.id = TOK_EOF;
		
		return TOK_EOF;
	}
	
	peek = ' ';
	lex_item.id = TOK_UNKNOWN;

	return TOK_UNKNOWN;
}

void
set_input(FILE *fp)
{
	input = fp;
}

FILE *
get_input()
{
	return input;
}

