#include <assert.h>
#include <errno.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "climits.h"
#include "common.h"
#include "lex.h"
#include "list.h"
#include "log.h"
#include "keyword.h"
#include "macros.h"
#include "syntax.h"
#include "variable.h"


/*******SYNTAX_CTX***********/

struct syn_ctx {
	int is_cycle;
	int is_cond;
	int is_func;
};

struct syn_ctx helper;

/*******SYNTAX_CTX***********/


static ast_node_t * global_expr();
static ast_node_t * stmts();
static ast_node_t *block();
static ast_node_t *statement();
static ast_node_t *expr();

static ast_node_t *assign(ast_node_t *first, int nesting);
static ast_node_t *get_rval();
static ast_node_t *op_with_assign(ast_node_t *lvalue);

static ast_node_t *trenary_op();
static ast_node_t *logic_or();
static ast_node_t *logic_and();
static ast_node_t *bool_or();
static ast_node_t *bool_xor();
static ast_node_t *bool_and();
static ast_node_t *equity();
static ast_node_t *rel_op();
static ast_node_t *shift_expr();

static ast_node_t *add_expr();
static ast_node_t *add_expr_rest();
static ast_node_t *mul_expr();
static ast_node_t *mul_expr_rest();
static ast_node_t *term();
static ast_node_t *unary_not();
static ast_node_t *unary_plus();
static ast_node_t *power();
static ast_node_t *concatenation();
static ast_node_t *factor();
static ast_node_t *identifier();

static ast_node_t *process_if();
static ast_node_t *process_for();
static ast_node_t *process_do();
static ast_node_t *process_while();
static ast_node_t *process_break();
static ast_node_t *process_continue();
static ast_node_t *process_del();

static ast_node_t *process_return();

static ast_node_t *process_scope();

static ast_node_t *process_import();

static ast_node_t *process_function();
static ret_t process_function_ret_lst(ast_node_func_t *func);
static ret_t process_function_arguments(ast_node_func_t *func);
static ret_t process_function_body(ast_node_func_t *func);

static ast_node_t *function_call();
static ast_node_t *array_access();
static ast_node_t *array_init();

extern struct lex_item lex_item_future;
extern struct lex_item lex_item;
extern struct lex_item lex_item_prev;
extern tok_t current_tok;


int syntax_is_eof = 0;

static int nerrors;

struct lex_item lex_item_prev;
struct lex_item lex_item_future;

tok_t current_tok;
int exist_stored_tokens = FALSE;

static void
update_token(struct lex_item *dst, const struct lex_item *src)
{
	dst->id = src->id;
	switch(src->id) {
	case TOK_ID:
		dst->name = src->name;
		break;
	case TOK_VAR:
		dst->var = src->var;
		break;
	default:
		break;
	}
}

static void
update_prev_token()
{
	update_token(&lex_item_prev, &lex_item);
}


static void
tok_next()
{
	if (exist_stored_tokens) {
		update_prev_token();
		update_token(&lex_item, &lex_item_future);
		current_tok = lex_item.id;
		exist_stored_tokens = FALSE;
	} else {
		// no buffered tokens
		update_prev_token();
		current_tok = get_next_token();
	}
}

/*
 * pops last token to token stream
 * WARN: only one pushback guaranteed
 */
static void
tok_pop()
{
	exist_stored_tokens = TRUE;

	update_token(&lex_item_future, &lex_item);
	update_token(&lex_item, &lex_item_prev);

	current_tok = lex_item.id;
}

static inline boolean_t
match(const tok_t expect)
{
	if (current_tok == expect) {
		tok_next();
		return TRUE;
	}

	return FALSE;
}

/*
 * try to skip next EOL,
 * returns TRUE if next EOL skipped
 * returns FALSE if current_tok not EOL
 */
static inline boolean_t
skip_eol()
{
	return match(TOK_EOL);
}

static inline void
sync_stream()
{
	while (current_tok != TOK_EOL &&
	    current_tok != TOK_EOF) {

		switch(current_tok) {
		case TOK_ID:
			ufree(lex_item.name);
			lex_item.name = NULL;
			break;
		case TOK_VAR:
			var_clear(lex_item.var);
			ufree(lex_item.var);
			lex_item.var = NULL;
		default:
			break;
		}

		tok_next();
	}
}

boolean_t
is_stmt_end() 
{
	switch (current_tok) {
	case TOK_EOL:
	case TOK_EOF:
	case TOK_SEMICOLON:
		return TRUE;
	default:
		return FALSE;
	}
}

boolean_t
is_eof()
{
	 if (current_tok == TOK_EOF) {
		syntax_is_eof = 1;
		return TRUE;
	} else
		return FALSE;
}

boolean_t
is_lvalue(ast_node_t *node)
{
	if (node->type != AST_NODE_ID &&
	    node->type != AST_NODE_ACCESS)
		return FALSE;

	return TRUE;
}

ret_t
program_start(ast_node_t **tree)
{
	DEBUG(LOG_VERBOSE, "\n");

	nerrors = 0;
	*tree = NULL;
	memset(&helper, 0, sizeof(helper));
	
	tok_next();

	*tree = global_expr();
	
	if (nerrors != 0) {
		//now we must flush tree
		ast_node_unref(*tree);
		*tree = NULL;

		return ret_err;
	}

	return ret_ok;
}

static ast_node_t *
global_expr()
{
	ast_node_t *node;

	DEBUG(LOG_VERBOSE, "\n");

	//Define new function
	if (match(TOK_DEF))
		return process_function();
	
	else if (match(TOK_IMPORT))
		return process_import();

	node = stmts();

	return node;
}

static ast_node_t *
stmts()
{
	DEBUG(LOG_VERBOSE, "\n");

	//return block();
	
	return statement();
}

static ast_node_t *
block()
{
	ast_node_t *result, *prev, *tmp;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = prev = NULL;

	//FIXME: bad, bad cycle
	while (is_eof() != TRUE) {

		//end of scope
		if (current_tok == TOK_RBRACE)
			break;
	
		tmp = statement();

		if (result == NULL)
			result = tmp;
		
		if (prev != NULL && tmp != NULL) {
			prev->child = tmp;
			tmp->parrent = prev;
		}

		if (tmp != NULL)
			prev = tmp;

		if (nerrors != 0)
			break;

		if (is_stmt_end() != TRUE) {
			nerrors++;
			sync_stream();
			print_warn("expected end of statement\n");
			break;
		}

		tok_next();
	}

	return result;
}

static ast_node_t *
statement()
{	
	DEBUG(LOG_VERBOSE, "\n");
	
	if (match(TOK_LBRACE)) 
		return process_scope();

	else if (match(TOK_IF))
		return process_if();
	
	else if (match(TOK_FOR))
		return process_for();
	
	else if (match(TOK_WHILE))
		return process_while();

	else if (match(TOK_BREAK))
		return process_break();

	else if (match(TOK_CONTINUE))
		return process_continue();
	
	else if (match(TOK_RETURN))
		return process_return();

	else if (match(TOK_DO))
		return process_do();
	
	else if (match(TOK_DEL))
		return process_del();

	else
		return expr();
}

//FIXME: REWRITEME!
static ast_node_t *
expr()
{
	DEBUG(LOG_VERBOSE, "\n");
	
	ast_node_t *result;
	
	result = trenary_op();
	if (result == NULL)
		return NULL;

	switch (current_tok) {
	case TOK_AS:
	case TOK_COMMA:
		return assign(result, 0);
	case TOK_PLUS_AS:
	case TOK_MINUS_AS:
	case TOK_MUL_AS:
	case TOK_DIV_AS:
	case TOK_POW_AS:
	case TOK_B_AND_AS:
	case TOK_B_XOR_AS:
	case TOK_B_OR_AS:
	case TOK_SHL_AS:
	case TOK_SHR_AS:
		return op_with_assign(result);
	default:
		return result;
	}
}

static ast_node_t *
op_with_assign(ast_node_t *node)
{
	assert(node != NULL);

	DEBUG(LOG_VERBOSE, "\n");
	
	ast_node_t *right;
	int op;
	
	right = NULL;

	if (!is_lvalue(node)) {
		print_warn("assign to not variable\n");
		goto err;
	}

	switch (current_tok) {
	case TOK_PLUS_AS:
		op = OP_PLUS;
		break;
	case TOK_MINUS_AS:
		op = OP_MINUS;
		break;
	case TOK_MUL_AS:
		op = OP_MUL;
		break;
	case TOK_DIV_AS:
		op = OP_DIV;
		break;
	case TOK_POW_AS:
		op = OP_POW;
		break;
	case TOK_B_AND_AS:
		op = OP_B_AND;
		break;
	case TOK_B_XOR_AS:
		op = OP_B_XOR;
		break;
	case TOK_B_OR_AS:
		op = OP_B_OR;
		break;
	case TOK_SHL_AS:
		op = OP_SHL;
		break;
	case TOK_SHR_AS:
		op = OP_SHR;
		break;
	default:
		error(1, "ERROR!\n");
	}
	tok_next();
	
	right = trenary_op();
	if (right == NULL) {
		print_warn("uncomplited as expression\n");
		goto err;
	}

	return ast_node_op_as_new(node, right, op);

err:
	nerrors++;

	ast_node_unref(right);
	
	return ast_node_stub_new();
}

static ast_node_t *
get_rval()
{
	ast_node_t *right;

	//to array
	if (match(TOK_LBRACE)) {
		right = array_init();
		if (right == NULL)
			goto err;
	} else {
		//to variable
		right = trenary_op();
		if (right == NULL) {
			print_warn("uncomplited as expression\n");
			goto err;
		}
	}

	return right;
err:
	nerrors++;
	ast_node_unref(right);
	return NULL;
}

//seq may include expressions or array initialisations
ast_node_t *
get_seq(ast_node_t *first)
{
	ast_node_seq_t *seq;
	ast_node_t *node;

	seq = (ast_node_seq_t *)ast_node_seq_new();
	ast_node_seq_add(seq, first);

	while (match(TOK_COMMA)) {
		node = get_rval();
		if (node == NULL || nerrors != 0) {
			print_warn("expression expected\n");
			goto err;
		}
		ast_node_seq_add(seq, node);
	}
	return AST_NODE(seq);
err:
	nerrors++;
	ast_node_unref(node);
	ast_node_unref((ast_node_t *)seq);
	return NULL;
}

boolean_t
is_lvalue_seq(ast_node_seq_t *seq)
{
	int i;

	for (i = 0; i < seq->n; i++)
		if (!is_lvalue(seq->node[i]))
			return FALSE;
	return TRUE;
}

// assign -> lval1 [, lval2 ...] = rval1 [, rval2 ...]
/* NOTE: rsz may be no equeal lsz (for example when user assign
 * function with multiple outputs
 */
//FIXME: может стоит переименовать 2 аргумент. подразумевал 'вложенность'
static ast_node_t *
assign(ast_node_t *node, int nesting)
{
	ast_node_t *lseq, *rseq;

	lseq = rseq = NULL;

	DEBUG(LOG_VERBOSE, "\n");

	// Get left sequnce
	lseq = get_seq(node);
	if (lseq == NULL || nerrors != 0)
		goto err;

	if (!match(TOK_AS)) {
		//was assign before?
		if (nesting > 0)
			return lseq;
		print_warn("'=' expected\n");
		goto err;
	}

	if (!is_lvalue_seq((ast_node_seq_t *)lseq)) {
		print_warn("can't assign to not lvalue\n");
		goto err;
	}

	node = get_rval();
	if (node == NULL || nerrors != 0)
		goto err;
	rseq = assign(node, nesting + 1);
	if (rseq == NULL || nerrors != 0)
		goto err;

	return ast_node_as_new(lseq, rseq);

err:
	ast_node_unref(lseq);
	ast_node_unref(rseq);

	return ast_node_stub_new();
}

static ast_node_t *
trenary_op()
{
	ast_node_t *_if_yes, *_if_no;
	ast_node_t *cond;

	DEBUG(LOG_VERBOSE, "\n");
	
	cond = logic_or();
	
	if (cond == NULL)
		return NULL;

	if (match(TOK_QUESTION) == FALSE)
		return cond;
	_if_yes = expr();
	if (match(TOK_COLON) == FALSE ||
	    _if_yes == NULL || nerrors != 0)
		goto err;
		
	_if_no = expr();
	if (_if_no == NULL || nerrors != 0)
		goto err;
	
	return ast_node_trenary_new(cond, _if_yes, _if_no);
err:
	nerrors++;
	ast_node_unref(cond);
	print_warn("trenary op syntax error\n");
	return ast_node_stub_new();
}

static ast_node_t *
logic_or()
{
	ast_node_t *right, *result;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = logic_and();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		if (match(TOK_L_OR) == FALSE)
			return result;

		right = logic_and();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited eq expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_L_OR);
	}
}

static ast_node_t *
logic_and()
{
	ast_node_t *right, *result;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = bool_or();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		if(match(TOK_L_AND) == FALSE)
			return result;
		
		right = bool_or();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited eq expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_L_AND);
	}
}

static ast_node_t *
bool_or()
{
	ast_node_t *right, *result;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = bool_xor();

	if (result == NULL)
		return NULL;

	while (TRUE) {
		if (match(TOK_B_OR) == FALSE)
			return result;

		right = bool_xor();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited bool or expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_B_OR);
	}
}

static ast_node_t *
bool_xor()
{
	ast_node_t *right, *result;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = bool_and();

	if (result == NULL)
		return NULL;

	while (TRUE) {
		if (match(TOK_B_XOR) == FALSE)
			return result;

		right = bool_and();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited bool or expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_B_XOR);
	}
}

static ast_node_t *
bool_and()
{
	ast_node_t *right, *result;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = equity();

	if (result == NULL)
		return NULL;

	while (TRUE) {
		if (match(TOK_B_AND) == FALSE)
			return result;

		right = equity();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited bool or expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_B_AND);
	}
	return result;
}

static ast_node_t *
equity()
{
	ast_node_t *right, *result;
	opcode_t op;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = rel_op();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		switch (current_tok) {
		case TOK_EQ:
			op = OP_EQ;
			break;
		case TOK_NEQ:
			op = OP_NEQ;
			break;
		default:
			return result;
		}
		tok_next();

		right = rel_op();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited eq expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, op);
	}
	return result;
}

static ast_node_t *
rel_op()
{
	ast_node_t *right, *result;
	int op;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = shift_expr();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		switch (current_tok) {
		case TOK_GR:
			op = OP_GR;
			break;
		case TOK_GE:
			op = OP_GE;
			break;
		case TOK_LO:
			op = OP_LO;
			break;
		case TOK_LE:
			op = OP_LE;
			break;
		default:
			return result;
		}
		tok_next();

		right = shift_expr();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited rel expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, op);
	}
}

static ast_node_t *
shift_expr()
{
	ast_node_t *right, *result;
	int op;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = add_expr();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		switch (current_tok) {
		case TOK_SHL:
			op = OP_SHL;
			break;
		case TOK_SHR:
			op = OP_SHR;
			break;
		default:
			return result;
		}
		tok_next();

		right = add_expr();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited shift expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, op);
	}
}


static ast_node_t *
add_expr()
{
	ast_node_t *tree;

	DEBUG(LOG_VERBOSE, "\n");
	
	tree = mul_expr();

	if (tree == NULL)
		return NULL;
	
	return add_expr_rest(tree);
}

ast_node_t *
add_expr_rest(ast_node_t *left)
{
	ast_node_t *right, *result;
	int op;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = left;

	while(TRUE) {
		switch (current_tok) {
		case TOK_PLUS:
			op = OP_PLUS;
			break;
		case TOK_MINUS:
			op = OP_MINUS;
			break;
		default:
			return result;
		}
		tok_next();

		right = mul_expr();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited expr expression\n");
			right = ast_node_stub_new();
		}

		result = ast_node_op_new(result, right, op);
	}
}

static ast_node_t *
mul_expr()
{
	ast_node_t *tree;

	DEBUG(LOG_VERBOSE, "\n");
	
	tree = term();
	if (tree == NULL) 
		return NULL;

	return mul_expr_rest(tree);
}

static ast_node_t *
mul_expr_rest(ast_node_t *left)
{
	ast_node_t *right, *result;
	int op;

	result = left;

	while(TRUE) {
		switch (current_tok) {
		case TOK_MUL:
			op = OP_MUL;
			break;
		case TOK_DIV:
			op = OP_DIV;
			break;
		case TOK_PERSENT:
			op = OP_MOD;
			break;
		default:
			return result;
		}
		tok_next();

		right = term();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited term expression\n");
			right = ast_node_stub_new();
		}

		result = ast_node_op_new(result, right, op);
	}
}

static ast_node_t *
term()
{
	if (current_tok == TOK_EOL
	    || current_tok == TOK_SEMICOLON) {
		
		return NULL;
	} else if (is_eof() == TRUE) {

		return NULL;
	} else if (current_tok == TOK_PLUS
	    || current_tok == TOK_MINUS) {
		
		return unary_plus();
	} else if (current_tok == TOK_NOT) {

		return unary_not();
	} else {

		return power();
	}
}

static ast_node_t *
unary_not()
{
	ast_node_t *node;

	int not;
	not = 0;

	do {
		if (current_tok == TOK_NOT)
			not = (not + 1) % 2;

	} while (match(TOK_NOT) == TRUE);

	node = power();
	
	if (not)
		return ast_node_unary_new(node, OP_NOT);
	else
		return ast_node_unary_new(node, OP_NOTNOT);
}

static ast_node_t *
unary_plus()
{
	ast_node_t *node;
	int minus;

	minus = 0;

	do {
		if (current_tok == TOK_MINUS)
			minus = (minus + 1) % 2;

	} while (match(TOK_PLUS) == TRUE || match(TOK_MINUS) == TRUE);

	node = power();

	if (minus)
		return ast_node_unary_new(node, OP_MINUS);
	else
		return ast_node_unary_new(node, OP_PLUS);
}

static ast_node_t *
power()
{
	ast_node_t *right, *result;
	int op;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = concatenation();

	if (result == NULL)
		return NULL;

	switch (current_tok) {
	case TOK_POW:
		op = OP_POW;
		break;
	default:
		return result;
	}
	tok_next();

	right = power();

	if (right == NULL) {
		nerrors++;
		print_warn("uncomplited shift expression\n");
		right = ast_node_stub_new();
	}

	return ast_node_op_new(result, right, op);
}

static ast_node_t *
concatenation()
{
	ast_node_t *right, *result;
	int op;

	DEBUG(LOG_VERBOSE, "\n");
	result = factor();

	if (result == NULL)
		return NULL;

	while (TRUE) {
		switch (current_tok) {
		case TOK_LPAR:
		case TOK_ID:
		case TOK_VAR:
			op = OP_STR_CONCAT;
			break;
		case TOK_HASH:
			op = OP_OCTSTR_CONCAT;
			tok_next();
			break;
		default:
			return result;
		}

		right = factor();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited shift expression\n");
			right = ast_node_stub_new();
		}

		result = ast_node_op_new(result, right, op);
	}
}


// May be need to return ast_node_stub_t when RBRACKET or RPAR missed?
static ast_node_t *
factor()
{
	ast_node_t *stat;

	DEBUG(LOG_VERBOSE, "\n");
	
	if (match(TOK_LPAR)) {
		stat = logic_or();
		
		if (match(TOK_RPAR) == FALSE) {
			print_warn("right parenthesis missed\n");
			nerrors++;
			return stat;
		}
		
		return stat;
	} else if (match(TOK_ID)) {
		
		return identifier();
	
	} else if (match(TOK_VAR)) {

		return ast_node_var_new(lex_item_prev.var);

	} else {
		nerrors++;
		print_warn("unsupported token tryed to factor\n");
		sync_stream();

		return ast_node_stub_new();
	}
}

static ast_node_t *
identifier()
{
	DEBUG(LOG_VERBOSE, "\n");
	
	switch (current_tok) {
	case TOK_LBRACKET:
		return array_access();
	case TOK_LPAR:
		return function_call();
	default:
		return ast_node_id_new(lex_item_prev.name);
	}
}

static ast_node_t *
process_if()
{
	ast_node_t *_if, *body, *_else;
	boolean_t skipped;
	int saved;

	DEBUG(LOG_VERBOSE, "\n");
	
	body = _if = _else = NULL;
	saved = helper.is_cond++;

	if (match(TOK_LPAR) == FALSE) {
		print_warn("'(' is missed\n");
		goto err;
	}

	//FIXME: check nerrors(now haven't time)
	_if = logic_or();
	if (_if == NULL) {
		print_warn("expected expression\n");
		goto err;
	}

	if (match(TOK_RPAR) == FALSE) {
		print_warn("')' is missed\n");
		goto err;
	}
	
	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statement();
	} else
		body = process_scope();
	
	if (nerrors != 0) {
		print_warn("cant proc cond body\n");
		goto err;
	}
	
	skipped = skip_eol();

	if (match(TOK_ELSE) == TRUE) {
		skip_eol();
		_else = statement();
		if (_else == NULL) {
			print_warn("cant get stmt after 'else'\n");
			goto err;
		}
	} else {
		//FIXME: rewrite me
		if (skipped)
			tok_pop();
	}

	if (nerrors != 0) {
		print_warn("processing if fail\n");
		goto err;
	}

	helper.is_cond = saved;
	return ast_node_if_new(_if, body, _else);

err:
	ast_node_unref(_if);
	ast_node_unref(body);
	helper.is_cond = saved;
	
	nerrors++;
	return ast_node_stub_new();
}

static ast_node_t *
process_for()
{
	ast_node_t *expr1, *expr2, *expr3;
	ast_node_t *body;

	DEBUG(LOG_VERBOSE, "\n");
	
	body = expr1 = expr2 = expr3 = NULL;

	if (match(TOK_LPAR) == FALSE) {
		print_warn("'(' is missed\n");
		goto err;
	}

	expr1 = expr();
	
	if (match(TOK_SEMICOLON) == FALSE) {
		print_warn("semicolon expected\n");
		goto err;
	}

	expr2 = logic_or();

	if (match(TOK_SEMICOLON) == FALSE) {
		print_warn("semicolon expected\n");
		goto err;
	}

	if (current_tok != TOK_RPAR)
		expr3 = expr();
	
	if (match(TOK_RPAR) == FALSE) {
		print_warn("')' is missed\n");
		goto err;
	}

	helper.is_cycle++;

	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statement();
	} else
		body = process_scope();
	
	helper.is_cycle--;

	if (nerrors != 0) {
		print_warn("errors happen\n");
		goto err;
	}

	return ast_node_for_new(expr1, expr2, expr3, body);

	err:

	ast_node_unref(expr1);
	ast_node_unref(expr2);
	ast_node_unref(expr3);

	return ast_node_stub_new();
}


static ast_node_t *
process_do()
{
	ast_node_t *cond, *body;

	DEBUG(LOG_VERBOSE, "\n");

	body = cond = NULL;

	
	helper.is_cycle++;

	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statement();
	} else
		body = process_scope();
	
	helper.is_cycle--;

	if (nerrors != 0) {
		print_warn("errors happen\n");
		goto err;
	}
	
	skip_eol();
	if (match(TOK_WHILE) == FALSE) {
		print_warn("expected keyword 'while'\n");
		goto err;
	}

	if (match(TOK_LPAR) == FALSE) {
		print_warn("'(' is missed\n");
		goto err;
	}

	cond = logic_or();

	if (match(TOK_RPAR) == FALSE) {
		print_warn("')' is missed\n");
		goto err;
	}

	return ast_node_do_new(cond, body);
err:
	ast_node_unref(body);
	ast_node_unref(cond);

	return ast_node_stub_new();
}

static ast_node_t *
process_while()
{
	ast_node_t *cond, *body;

	DEBUG(LOG_VERBOSE, "\n");
	
	body = cond = NULL;
	
	if (match(TOK_LPAR) == FALSE) {
		print_warn("'(' is missed\n");
		goto err;
	}

	//FIXME: need to handle (void) and print appropriate
	//error message.
	cond = logic_or();

	if (match(TOK_RPAR) == FALSE) {
		print_warn("')' is missed\n");
		goto err;
	}

	helper.is_cycle++;

	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statement();
	} else
		body = process_scope();
	
	helper.is_cycle--;

	if (nerrors != 0) {
		print_warn("errors happen\n");
		goto err;
	}

	return ast_node_while_new(cond, body);

err:

	ast_node_unref(cond);

	return ast_node_stub_new();
}

static ast_node_t *
process_del()
{
	ast_node_t *node = NULL;

	DEBUG(LOG_VERBOSE, "\n");

	//FIXME: programmer may delete multiple identifiers with one statement.
	if (match(TOK_ID) == FALSE)
		goto err;

	node = identifier();
	
	if (!is_lvalue(node))
		goto err;

	return ast_node_del_new(node);

err:
	sync_stream();

	print_warn("can't delete not identifier\n");
	nerrors++;
	ast_node_unref(node);

	return ast_node_stub_new();
}

static ast_node_t *
process_break()
{
	DEBUG(LOG_VERBOSE, "\n");

	if (helper.is_cycle == 0) {
		print_warn("break outside cycle\n");
		nerrors++;
		return ast_node_stub_new();
	}
	
	return ast_node_break_new();
}

static ast_node_t *
process_continue()
{
	if (helper.is_cycle == 0) {
		print_warn("continue outside cycle\n");
		nerrors++;
		return ast_node_stub_new();
	}
	
	return ast_node_continue_new();
}

static ast_node_t *
process_return()
{
	if (helper.is_func == 0) {
		print_warn("return not in function\n");
		nerrors++;
		sync_stream();
		return ast_node_stub_new();
	}

	return ast_node_return_new();
}

static ast_node_t *
process_scope()
{
	ast_node_t *node;
	
	node = block();
	if (nerrors != 0) 
		goto err;
	
	if (match(TOK_RBRACE) == FALSE) {
		nerrors++;
		goto err;
	}

	return ast_node_scope_new(node);
err:
	
	ast_node_unref(node);
	return ast_node_stub_new();
}

char *
get_module_name()
{
	struct variable *var;
	str_t *str;
	char *modname;

	if (!match(TOK_VAR))
		return NULL;

	var = lex_item_prev.var;
	str = var_str_ptr(var);
	
	modname =  strdup_or_die(str_ptr(str));

	var_clear(var);
	ufree(var);

	return modname;
}

static ast_node_t *
process_import()
{
	FILE *fd, *prev;
	ast_node_t *rtree, **nodes;
	char *modname;
	int i, sz, len;
	int ret;

	nodes = NULL;
	sz = len = 0;
	
	modname = get_module_name();

	if (modname == NULL) {
		print_warn("Can't get module name.\n");
		goto err;
	}

	/* 
	 * NOTE: Need to memorize import file name
	 * because we wan't infinitie includes.
	 */

	fd = fopen(modname, "r");

	if (fd == NULL) {
		char *msg;
		msg = strerror(errno);
		print_warn("%s\n", msg);
		goto err;
	}

	prev = get_input();

	set_input(fd);

	while (is_eof() != TRUE) {
		ret_t ret;
		ret = program_start(&rtree);
		if (ret != ret_ok) {
			print_warn("module syntax error\n");
			goto err;
		}
		if (rtree == NULL)
			continue;

		if (len >= sz) {
			sz += 8;
			nodes = xrealloc(nodes, sizeof(*nodes) * sz);
		}
		nodes[len++] = rtree;
	}

	set_input(prev);

	//FIXME: May be need global ctx for nerrors?
	//Need to restore current_tok
	syntax_is_eof = 0;

	ret = fclose(fd);
	if (ret != 0) {
		print_warn("Can't close input file\n");
	}

	return ast_node_import_new(nodes, len);

err:
	if (fd != NULL) {
		set_input(prev);

		ret = fclose(fd);
		if (ret != 0) {
			print_warn("Can't close input file\n");
		}
	}

	sync_stream();
	for (i = 0; i < len; i++)
		ast_node_unref(nodes[i]);
	
	ufree(modname);
	ufree(nodes);

	nerrors++;
	return ast_node_stub_new();
}

//parse comma separated list of arguments and sets dst, nargs variables
//stops when finds tok endtok
//returns:
// FALSE if saw invalid token
// TRUE if all ok
boolean_t
get_name_arr(char ***dst, int *nargs, int limit, char *errmsg, int endtok)
{
	int i, n;
	char *id;

	n = 0;
	*dst = NULL;

	while (current_tok != endtok) {
		if (match(TOK_ID) == FALSE) {
			print_warn("identifier required\n");
			goto err;
		}

		if (n + 1 > limit) {
			print_warn("%s: %d is maximum\n", errmsg, limit);
			goto err;
		}

		id = lex_item_prev.name;
		
		*dst = xrealloc(*dst, (n + 1) * sizeof(**dst));
		(*dst)[n++] = id;
		
		if (match(TOK_COMMA) == FALSE &&
		    current_tok != endtok) {
			print_warn("comma expected\n");
			goto err;
		}
	}
	*nargs = n;

	return TRUE;
err:
	for (i = 0; i < *nargs; i++)
		ufree((*dst)[i]);
	ufree(*dst);
	*dst = NULL;
	*nargs = 0;

	return FALSE;
}

static ast_node_t *
process_function()
{
	ast_node_func_t *func;
	int ret;

	func = (ast_node_func_t *)ast_node_func_def_new();
	
	ret = process_function_ret_lst(func);
	if (ret != ret_ok)
		goto err;

	if (match(TOK_ID) == FALSE) {
		print_warn("you must set function name"
		    " at initialisation\n");
		goto err;
	}

	func->name = lex_item_prev.name;
	
	ret = process_function_arguments(func);
	if (ret != ret_ok)
		goto err;

	ret = process_function_body(func);
	if (ret != ret_ok)
		goto err;
	
	return AST_NODE(func);

err:

	nerrors++;
	//do cleanup
	ast_node_unref(AST_NODE(func));
	sync_stream();

	return ast_node_stub_new();
}

static ret_t 
process_function_ret_lst(ast_node_func_t *func)
{
	if (!match(TOK_LBRACKET)) {
		print_warn("you must set function return list"
		    " at initialisation\n");
		return ret_err;
	}

	if (!get_name_arr(&func->retargs, &func->nret, MAXRETARGS,
	    "function return arguments:", TOK_RBRACKET))
		return ret_err;

	if (!match(TOK_RBRACKET)) {
		print_warn("']' expected\n");
		return ret_err;
	}
	return ret_ok;
}

//need to flush token stream after errrors
//
static ret_t
process_function_arguments(ast_node_func_t *func)
{
	if (match(TOK_LPAR) == FALSE) {
		print_warn("you must set parameter list"
		    "at initialisation\n");
		return ret_err;
	}

	//get arguments
	if (!get_name_arr(&func->args, &func->nargs, MAXFARGS,
	    "function arguments", TOK_RPAR))
		return ret_err;

	if (!match(TOK_RPAR)) {
		print_warn("')' expected\n");
		return ret_err;
	}
	return ret_ok;
}

static ret_t
process_function_body(ast_node_func_t *func)
{
	if (match(TOK_LBRACE) == FALSE) {
		print_warn("LBRACE required\n");
		goto err;
	}

	helper.is_func++;

	//WARN: void body functions
	func->body = block();
	if (nerrors != 0) {
		print_warn("cant traverse func body\n");
		goto err;
	}

	if (match(TOK_RBRACE) == FALSE) {
		print_warn("'}' missing\n");
		goto err;
	}

	helper.is_func--;
	
	return ret_ok;

err:

	nerrors++;
	ast_node_unref(func->body);
	func->body = NULL;

	return ret_err;
}
		
static ast_node_t *
function_call()
{
	ast_node_func_call_t *call;
	ast_node_t *node;
	char *name;

	name = lex_item_prev.name;
	call = (ast_node_func_call_t *) ast_node_func_call_new(name);

	if (match(TOK_LPAR) == FALSE) {
		print_warn("you must set parameter list"
		    "at initialisation\n");
		goto err;
	}

	while (match(TOK_RPAR) == FALSE) {
		
		//get next ast_node
		node = logic_or();
		if (node == NULL) {
			print_warn("expected statement");
			goto err;
		}

		ast_node_func_call_add_next_arg(call, node);

		if (match(TOK_COMMA) == FALSE &&
		    current_tok != TOK_RPAR) {
			print_warn("comma expected\n");
			goto err;
		}
	}
	
	return AST_NODE(call);

err:

	nerrors++;
	ast_node_unref(AST_NODE(call));
	
	return ast_node_stub_new();
}

static ast_node_t *
array_access()
{
	ast_node_t **ind, *item;
	char *name;
	int i, dim, sz;

	ind = NULL;
	sz = dim = 0;

	name = lex_item_prev.name;
	tok_next();
	
	do {
		item = logic_or();
		if (item == NULL) {
			nerrors++;
			print_warn("null expression in brackets");
			return ast_node_stub_new();
		}

		if (dim >= sz) {
			sz += 4;
			ind = xrealloc(ind, sz * sizeof(*ind));
		}
		
		ind[dim++] = item;

	} while (match(TOK_COMMA));

	if (!match(TOK_RBRACKET)) {
		print_warn("']' expected\n");
		goto error;
	}

	return ast_node_access_new(name, dim, ind);

error:
	for (i = 0; i < dim; i++)
		ast_node_unref(ind[i]);

	ufree(ind);

	nerrors++;
	return ast_node_stub_new();
}

/*
 * try to init new ast_node_arr_t
 * print warn and return NULL if some error occured
 * return ast_node_arr_t otherwise
 */
static ast_node_t *
array_init()
{
	ast_node_t *item, **arr;

	int i, total, arrsz;
	
	arr = NULL;
	total = arrsz = 0;

	do {
		item = logic_or();
		if (item == NULL) {
			print_warn("uncomplited tuple\n");
			goto error;
		}
		if (total >= arrsz) {
			arrsz += 4;
			arr = xrealloc(arr, arrsz * sizeof (*arr));
		}

		arr[total++] = item;

	} while (match(TOK_COMMA));

	if (!match(TOK_RBRACE)) {
		print_warn("'}' expected\n");
		goto error;
	}

	if (total == 0) {
		print_warn("empty scalar initializer\n");
		goto error;
	}

	return ast_node_arr_new(arr, total);

error:
	sync_stream();

	for (i = 0; i < total; i++)
		ast_node_unref(arr[i]);
	
	ufree(arr);
	return NULL;
}

