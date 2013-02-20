#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <mpl.h>

#include "keyword.h"
#include "lex.h"
#include "macros.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"


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

static void
skip_multiline_comment()
{
	while (TRUE) {
		peek = fgetc(input);
		if (peek != '*')
			continue;
		peek = fgetc(input);
		if (peek != '/')
			continue;
		peek = ' ';
		return;
	}
}

static tok_t
get_string()
{
	str_t *str;
	struct variable *var;
	char *s = NULL;
	char *tmp;
	int len, used;

	var = xmalloc(sizeof(*var));
	var_init(var);

	str = var_str_ptr(var);

	used = 0;
	len = 64;
	s = xmalloc(len);

	peek = fgetc(input);

	while (peek != EOF && peek != '"') {

		if (used >= len - 1) {
			len += 64;
			s = xrealloc(s, len);
		}

		s[used++] = peek;
		peek = fgetc(input);
	}

	if (peek != '"') {
		print_warn("uncomplited string");
		ufree(s);
		peek = ' ';
		return TOK_UNKNOWN;
	}

	s[used++] = '\0';

	if ((tmp = realloc(s, used)) == NULL)
		error(1, "realloc_err");
	s = tmp;

	str_append(str, s);

	peek = ' ';

	ufree(s);

	lex_item.id = TOK_VAR;
	lex_item.var = var;
	var_force_type(var, VAR_STRING);

	return TOK_VAR;
}

/*
 * returns:
 * 0 if convertion success
 * 1 otherwise
 */

int
convert_to_digit_base(char ch, int *num, int base)
{
	int tmp;
	int lc;
	
	lc = tolower(ch);

	if (lc >= '0' && lc <= '9')
		tmp = lc - '0';
	else if(lc >= 'a' && lc <= 'z')
		tmp = tolower(ch) - 'a' + 10;
	else
		return 1;

	if (tmp >= base || tmp < 0)
		return 1;
	else
		*num = tmp;

	return 0;
}

static void
get_digit_base(mpl_int *mp, int base)
{
	mpl_int tmp, mpbase;
	int num;

	mpl_initv(&tmp, &mpbase, NULL);

	mpl_zero(mp);

	mpl_set_uint(&mpbase, base);

	while (convert_to_digit_base(peek, &num, base) == 0) {
		/* mp = mp * STR_BASE + peek - '0'; */
		mpl_set_uint(&tmp, num);

		mpl_mul(mp, mp, &mpbase); 
		mpl_add(mp, mp, &tmp);
		
		peek = fgetc(input);
	};

	mpl_clearv(&tmp, &mpbase, NULL);
}

static tok_t
get_digit()
{
	mpl_int *mp;
	struct variable *var;

	var = xmalloc(sizeof(*var));
	var_initv(var, NULL);
	
	mp = var_bignum_ptr(var);

	if (peek == '0') {
		peek = fgetc(input);
		if (tolower(peek) == 'x') {
			peek = fgetc(input);
			get_digit_base(mp, 16);
		} else {
			get_digit_base(mp, 8);
		 }
	} else {
		get_digit_base(mp, STR_BASE);
	}

	lex_item.id = TOK_VAR;
	lex_item.var = var;
	var_force_type(var, VAR_BIGNUM);

#if IS_DEBUG == 1 && LOG_LEVEL == LOG_VERBOSE
#define MAX_NUM_SZ 200
	char templ_dig[MAX_NUM_SZ];

	rc = mpl_to_str(mp, templ_dig, MAX_NUM_SZ, STR_BASE);
	if (rc == MPL_OK)
		DEBUG(LOG_VERBOSE, "get_digit: %s\n", templ_dig);
#endif

	return TOK_VAR;
}

tok_t
get_next_token()
{

begin:

	for (; peek == ' ' || peek == '\t';)
		peek = fgetc(input);
	
	if (isdigit(peek))
		return get_digit();

	if (isalpha(peek) || peek == '_') {
		tok_t kword;
		char *s = NULL;
		char *tmp;
		int used, len;
		
		len = used = 0;
		do {
			if (used >= len - 1) {
				len += 64;
				s = xrealloc(s, len);
			}

			s[used++] = peek;
			peek = fgetc(input);
		} while (isalnum(peek) || peek == '_');

		s[used++] = '\0';
		if ((tmp = realloc(s, used)) == NULL)
			error(1, "realloc_err");
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
		} else if (peek == '*') {
			peek = fgetc(input);
			if (peek == '=') {
				peek = ' ';
				lex_item.id = TOK_POW_AS;
				return TOK_POW_AS;
			}

			lex_item.id = TOK_POW;
			return TOK_POW;
		}
		lex_item.id = TOK_MUL;
		
		return TOK_MUL;
	case '/':
		peek = fgetc(input);
		if (peek == '=') {
			peek = ' ';
			lex_item.id = TOK_DIV_AS;
			return TOK_DIV_AS;
		} else if (peek == '/') {
			skip_comment();
			goto begin;
		} else if (peek == '*') {
			skip_multiline_comment();
			goto begin;
		}
		lex_item.id = TOK_DIV;

		return TOK_DIV;
	case '#':
		peek = ' ';
		lex_item.id = TOK_HASH;

		return TOK_HASH;
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

