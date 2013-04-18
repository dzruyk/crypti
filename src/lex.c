#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <mpl.h>

#include "common.h"
#include "keyword.h"
#include "lex.h"
#include "macros.h"
#include "octstr.h"
#include "str.h"
#include "variable.h"


static char peek = ' ';

/* current input file */
FILE *input;

boolean_t convert_to_digit_base(char ch, int *num, int base);

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

boolean_t
get_hex_char(char *res)
{
	int a, b;

	peek = fgetc(input);

	if (!convert_to_digit_base(peek, &a, 16)) {
		ungetc(peek, input);
		return FALSE;
	}
	peek = fgetc(input);
	if (!convert_to_digit_base(peek, &b, 16)) {
		ungetc(peek, input);
		return FALSE;
	}

	*res = (char)(a * 16 + b);
	return TRUE;
}

boolean_t
get_unescaped_char(char *res, char peek)
{
	if (peek == '\\') {
		peek = fgetc(input);
		switch (peek) {
		case '\\':
			*res = '\\';
			break;
		case '`':
			*res = '`';
			break;
		case 'x':
			if (!get_hex_char(res))
				return FALSE;
			break;
		default:
			*res = peek;
			break;
		}
	} else {
		*res = peek;
	}

	return TRUE;
}

static tok_t
get_octstring()
{
	octstr_t *octstr;
	struct variable *var;
	char *s = NULL;
	char *tmp;
	int len, used;

	var = xmalloc(sizeof(*var));
	var_init(var);

	octstr = var_octstr_ptr(var);

	used = 0;
	len = 64;
	s = xmalloc(len);

	peek = fgetc(input);

	while (peek != EOF && peek != '`') {

		if (used >= len - 1) {
			len += 64;
			s = xrealloc(s, len);
		}

		if (!get_unescaped_char(&s[used], peek)) {
			print_warn("escape error\n");
			goto err;
		}
		peek = fgetc(input);
		used++;
	}

	if (peek != '`') {
		print_warn("uncomplited octstring\n");
		goto err;
	}

	tmp = xrealloc(s, used);
	s = tmp;

	octstr_append_n(octstr, s, used);

	peek = ' ';

	ufree(s);

	lex_item.id = TOK_VAR;
	lex_item.var = var;
	var_force_type(var, VAR_OCTSTRING);

	return TOK_VAR;
err:
	ufree(s);
	peek = ' ';
	return TOK_UNKNOWN;
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
 * true if convertion success
 * false otherwise
 */
boolean_t
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
		return FALSE;

	if (tmp >= base || tmp < 0)
		return FALSE;
	else
		*num = tmp;

	return TRUE;
}

static void
get_digit_base(mpl_int *mp, int base)
{
	mpl_int tmp, mpbase;
	int num;

	mpl_initv(&tmp, &mpbase, NULL);

	mpl_zero(mp);

	mpl_set_uint(&mpbase, base);

	while (convert_to_digit_base(peek, &num, base)) {
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
	if (peek == '`')
		return get_octstring();

	switch (peek) {

#define CASE_ITEM(ch, tok_type)			\
case ch:					\
	lex_item.id = tok_type;			\
	goto clean_end				\

	CASE_ITEM('(', TOK_LPAR);
	CASE_ITEM(')', TOK_RPAR);
	CASE_ITEM('[', TOK_LBRACKET);
	CASE_ITEM(']', TOK_RBRACKET);
	CASE_ITEM('{', TOK_LBRACE);
	CASE_ITEM('}', TOK_RBRACE);
	CASE_ITEM(',', TOK_COMMA);
	CASE_ITEM(';', TOK_SEMICOLON);
	CASE_ITEM('#', TOK_HASH);
	CASE_ITEM('.', TOK_DOT);
	CASE_ITEM('\n', TOK_EOL);
	CASE_ITEM(EOF, TOK_EOF);
	CASE_ITEM('%', TOK_PERSENT);

	case '=':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_EQ;
			goto clean_end;
		}
		lex_item.id = TOK_AS;
		
		return TOK_AS;
	case '!':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_NEQ;
			goto clean_end;
		}
		lex_item.id = TOK_NOT;

		return TOK_NOT;
	case '<':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_LE;
			goto clean_end;
		} else if (peek == '<') {
			peek = fgetc(input);
			if (peek == '=') {
				lex_item.id = TOK_SHL_AS;
				goto clean_end;
			}
			lex_item.id = TOK_SHL;

			return TOK_SHL;
		}
		lex_item.id = TOK_LO;

		return TOK_LO;
	case '>':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_GE;
			goto clean_end;
		} else if (peek == '>') {
			peek = fgetc(input);
			if (peek == '=') {
				lex_item.id = TOK_SHR_AS;
				goto clean_end;
			}
			lex_item.id = TOK_SHR;

			return TOK_SHR;
		}
		lex_item.id = TOK_GR;

		return TOK_GR;
	case '&':
		peek = fgetc(input);
		if (peek == '&') {
			lex_item.id = TOK_L_AND;
			goto clean_end;
		} else if (peek == '=') {
			lex_item.id = TOK_B_AND_AS;
			goto clean_end;
		}
		lex_item.id = TOK_B_AND;

		return TOK_B_AND;
	case '|':
		peek = fgetc(input);
		if (peek == '|') {
			lex_item.id = TOK_L_OR;
			goto clean_end;
		} else if (peek == '=') {
			lex_item.id = TOK_B_OR_AS;
			goto clean_end;
		}
		lex_item.id = TOK_B_OR;

		return TOK_B_OR;
	case '^':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_B_XOR_AS;
			goto clean_end;
		}
		lex_item.id = TOK_B_XOR;

		return TOK_B_XOR;
	case '+':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_PLUS_AS;
			goto clean_end;
		}
		lex_item.id = TOK_PLUS;
		
		return TOK_PLUS;
	case '-':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_MINUS_AS;
			goto clean_end;
		}
		lex_item.id = TOK_MINUS;
		
		return TOK_MINUS;
	case '*':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_MUL_AS;
			goto clean_end;
		} else if (peek == '*') {
			peek = fgetc(input);
			if (peek == '=') {
				lex_item.id = TOK_POW_AS;
				goto clean_end;
			}
			lex_item.id = TOK_POW;

			return TOK_POW;
		}
		lex_item.id = TOK_MUL;
		
		return TOK_MUL;
	case '/':
		peek = fgetc(input);
		if (peek == '=') {
			lex_item.id = TOK_DIV_AS;
			goto clean_end;
		} else if (peek == '/') {
			skip_comment();
			goto begin;
		} else if (peek == '*') {
			skip_multiline_comment();
			goto begin;
		}
		lex_item.id = TOK_DIV;

		return TOK_DIV;
	default:
		lex_item.id = TOK_UNKNOWN;
		goto clean_end;
	}

clean_end:	
	peek = ' ';
	return lex_item.id;
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

