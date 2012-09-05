#include <assert.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "crypti.h"
#include "keyword.h"
#include "macros.h"
#include "syntax.h"
#include "lex.h"
#include "list.h"

/*******SYNTAX_CTX***********/

enum ctx_type {
	CTX_FUNCTION,
	CTX_GLOBAL,
	CTX_SCOPE,
	CTX_UNKNOWN,
};

struct syn_ctx {
	int type;
	int is_cycle;
	int is_cond;
};

struct syn_ctx *syn_ctx_new();


struct syn_ctx *
syn_ctx_new()
{
	struct syn_ctx *ctx;

	ctx = xmalloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));

	return ctx;
}

void
syn_ctx_free(struct syn_ctx *ctx)
{
	free(ctx);
}

/*******SYNTAX_CTX***********/


static ast_node_t * global_expr();
static ast_node_t * stmts(struct syn_ctx *ctx);
static ast_node_t *block(struct syn_ctx *ctx);
static ast_node_t *statesment(struct syn_ctx *ctx);
static ast_node_t *expr();

static ast_node_t *assign(ast_node_t *lvalue);
static ast_node_t *op_with_assign(ast_node_t *lvalue);

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
static ast_node_t *factor();
static ast_node_t *identifier();

static ast_node_t *process_if(struct syn_ctx *ctx);
static ast_node_t *process_for(struct syn_ctx *ctx);
static ast_node_t *process_do(struct syn_ctx *ctx);
static ast_node_t *process_while(struct syn_ctx *ctx);
static ast_node_t *process_break(struct syn_ctx *ctx);
static ast_node_t *process_continue(struct syn_ctx *ctx);

static ast_node_t *process_return(struct syn_ctx *ctx);

static ast_node_t *process_scope(struct syn_ctx *ctx);

static ast_node_t *process_import();

static ast_node_t *process_function();
static ret_t process_function_argu(ast_node_func_t *func);
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
	case TOK_NUM:
		dst->num = src->num;
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

ret_t
program_start(ast_node_t **tree)
{
	DEBUG(LOG_VERBOSE, "\n");

	nerrors = 0;
	*tree = NULL;
	
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
	struct syn_ctx *ctx;

	DEBUG(LOG_VERBOSE, "\n");

	//Define new function
	if (match(TOK_DEF))
		return process_function();
	
	else if (match(TOK_IMPORT))
		return process_import();

	ctx = syn_ctx_new();
	ctx->type = CTX_GLOBAL;

	node = stmts(ctx);

	syn_ctx_free(ctx);

	return node;
}

static ast_node_t *
stmts(struct syn_ctx *ctx)
{
	DEBUG(LOG_VERBOSE, "\n");

	//if (ctx->type != CTX_GLOBAL)
	return block(ctx);
	
	//return statesment(ctx);
}

static ast_node_t *
block(struct syn_ctx *ctx)
{
	ast_node_t *result, *prev, *tmp;

	DEBUG(LOG_VERBOSE, "\n");
	
	result = prev = NULL;

	//FIXME: bad, bad cycle
	while (is_eof() != TRUE) {

		//end of scope
		if (current_tok == TOK_RBRACE)
			break;
	
		tmp = statesment(ctx);

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
			print_warn("expected end of statesment\n");
			break;
		}

		//FIXME: rewrite me
		//mb implement CTX_SCOPE?
		if (ctx->type == CTX_GLOBAL)
			break;

		tok_next();
	}

	return result;
}

static ast_node_t *
statesment(struct syn_ctx *ctx)
{	
	DEBUG(LOG_VERBOSE, "\n");
	
	if (match(TOK_LBRACE)) 
		return process_scope(ctx);

	else if (match(TOK_IF))
		return process_if(ctx);
	
	else if (match(TOK_FOR))
		return process_for(ctx);
	
	else if (match(TOK_WHILE))
		return process_while(ctx);

	else if (match(TOK_BREAK))
		return process_break(ctx);

	else if (match(TOK_CONTINUE))
		return process_continue(ctx);
	
	else if (match(TOK_RETURN))
		return process_return(ctx);

	else if (match(TOK_DO))
		return process_do(ctx);

	else
		return expr();
}

//FIXME: REWRITEME!
static ast_node_t *
expr()
{
	DEBUG(LOG_VERBOSE, "\n");
	
	ast_node_t *result;
	
	result = logic_or();
	if (result == NULL)
		return NULL;

	if (match(TOK_AS) == TRUE)
		return assign(result);

	switch (current_tok) {
	case TOK_PLUS_AS:
	case TOK_MINUS_AS:
	case TOK_MUL_AS:
	case TOK_DIV_AS:
	case TOK_B_AND_AS:
	case TOK_B_XOR_AS:
	case TOK_B_OR_AS:
	case TOK_SHL_AS:
	case TOK_SHR_AS:
		return op_with_assign(result);
		break;
	default:
		return result;
	}
}

static ast_node_t *
op_with_assign(ast_node_t *lvalue)
{
	assert(lvalue != NULL);

	DEBUG(LOG_VERBOSE, "\n");
	
	ast_node_t *right;
	int op;
	
	right = NULL;

	if (lvalue->type != AST_NODE_ID &&
	    lvalue->type != AST_NODE_ACCESS) {
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
		print_warn_and_die("ERROR!\n");
	}
	tok_next();
	
	right = logic_or();
	if (right == NULL) {
		print_warn("uncomplited as expression\n");
		goto err;
	}

	return ast_node_op_as_new(lvalue, right, op);

err:
	nerrors++;

	ast_node_unref(right);
	
	return ast_node_stub_new();
}

static ast_node_t *
assign(ast_node_t *lvalue)
{
	ast_node_t *right = NULL;
	
	DEBUG(LOG_VERBOSE, "\n");
	
	//FIXME: mb need to write special function at future?
	if (lvalue->type != AST_NODE_ID &&
	    lvalue->type != AST_NODE_ACCESS) {
		print_warn("assign to not variable\n");
		goto err;
	}

	//to array
	if (current_tok == TOK_LBRACE) {
		right = array_init();
		if (right == NULL)
			goto err;

		return ast_node_as_new(lvalue, right);
	}

	//to number
	right = expr();
	if (right == NULL) {
		print_warn("uncomplited as expression\n");
		goto err;
	}

	return ast_node_as_new(lvalue, right);

err:
	nerrors++;
	ast_node_unref(right);
	ast_node_unref(lvalue);
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

		return factor();
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

	node = factor();
	
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

	node = factor();

	if (minus)
		return ast_node_unary_new(node, OP_MINUS);
	else
		return ast_node_unary_new(node, OP_PLUS);
}

// May be need to return ast_node_stub_t when RBRACKET or RPAR missed?
static ast_node_t *
factor()
{
	ast_node_t *stat;

	DEBUG(LOG_VERBOSE, "\n");
	
	if (match(TOK_LPAR)) {
		stat = expr();
		
		if (match(TOK_RPAR) == FALSE) {
			print_warn("right parenthesis missed\n");
			nerrors++;
			return stat;
		}
		
		return stat;
	} else if (match(TOK_ID)) {
		
		return identifier();
	
	} else if (match(TOK_NUM)) {

		return ast_node_num_new(lex_item_prev.num);

	} else {
		nerrors++;
		print_warn("unsupported token tryed to factor\n");
		tok_next();
		
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
process_if(struct syn_ctx *ctx)
{
	ast_node_t *_if, *body, *_else;
	boolean_t skipped;

	DEBUG(LOG_VERBOSE, "\n");
	
	body = _if = _else = NULL;

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
		body = statesment(ctx);
	} else
		body = process_scope(ctx);
	
	if (nerrors != 0) {
		print_warn("cant proc cond body\n");
		goto err;
	}
	
	skipped = skip_eol();

	if (match(TOK_ELSE) == TRUE) {
		skip_eol();
		_else = statesment(ctx);
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

	return ast_node_if_new(_if, body, _else);

err:
	ast_node_unref(_if);
	ast_node_unref(body);
	
	nerrors++;
	return ast_node_stub_new();
}

static ast_node_t *
process_for(struct syn_ctx *ctx)
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

	ctx->is_cycle++;

	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statesment(ctx);
	} else
		body = process_scope(ctx);
	
	ctx->is_cycle--;

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
process_do(struct syn_ctx *ctx)
{
	ast_node_t *cond, *body;

	DEBUG(LOG_VERBOSE, "\n");

	body = cond = NULL;
	
	ctx->is_cycle++;

	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statesment(ctx);
	} else
		body = process_scope(ctx);
	
	ctx->is_cycle--;

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
process_while(struct syn_ctx *ctx)
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

	ctx->is_cycle++;

	if (match(TOK_LBRACE) == FALSE) {
		skip_eol();
		body = statesment(ctx);
	} else
		body = process_scope(ctx);
	
	ctx->is_cycle--;

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
process_break(struct syn_ctx *ctx)
{
	DEBUG(LOG_VERBOSE, "\n");

	if (ctx->is_cycle == 0) {
		print_warn("break outside cycle\n");
		nerrors++;
		return ast_node_stub_new();
	}
	
	return ast_node_break_new();
}

static ast_node_t *
process_continue(struct syn_ctx *ctx)
{
	if (ctx->is_cycle == 0) {
		print_warn("continue outside cycle\n");
		nerrors++;
		return ast_node_stub_new();
	}
	
	return ast_node_continue_new();
}

static ast_node_t *
process_return(struct syn_ctx *ctx)
{
	ast_node_t *node;

	if (ctx->type != CTX_FUNCTION) {
		print_warn("return not in function\n");
		nerrors++;
		return ast_node_stub_new();
	}

	node = expr();

	return ast_node_return_new(node);
}

static ast_node_t *
process_scope(struct syn_ctx *ctx)
{
	ast_node_t *node;
	int prev_type;
	
	//FIXME: I do this without new allocated ctx.
	prev_type = ctx->type;
	ctx->type = CTX_SCOPE;

	node = block(ctx);
	if (nerrors != 0) 
		goto err;
	
	if (match(TOK_RBRACE) == FALSE) {
		nerrors++;
		goto err;
	}

	ctx->type = prev_type;

	return ast_node_scope_new(node);
err:
	ctx->type = prev_type;
	
	ast_node_unref(node);
	return ast_node_stub_new();
}

char *
get_module_name()
{
	
	return NULL;
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
		print_warn("%s", msg);
		goto err;
	}

	prev = get_input();

	set_input(fd);

	 tok_next();
	 /* while not EOF
	 *	get_tree;
	 *	push_new_node
	 */
	 while (is_eof() != TRUE) {
		ret_t ret;
		ret = program_start(&rtree);
		if (ret != ret_ok) {
			print_warn("module syntax error\n");
			goto err;
		}

		if (len >= sz) {
			sz += 8;
			nodes = realloc_or_die(nodes, sizeof(*nodes) * sz);
		}
		nodes[len++] = rtree;
	 }

	set_input(prev);
	//FIXME: May be need global ctx for nerrors?
	syntax_is_eof = 0;

	ret = fclose(fd);
	if (ret != 0) {
		print_warn("Can't close input file\n");
	}

	//return new import node
	return ast_node_import_new(nodes, len);

err:
	for (i = 0; i < len; i++)
		ast_node_unref(nodes[i]);
	ufree(nodes);

	nerrors++;
	return ast_node_stub_new();
}

static ast_node_t *
process_function()
{
	ast_node_func_t *func;
	int ret;

	if (match(TOK_ID) == FALSE) {
		nerrors++;
		print_warn("you must set function name"
		    " at initialisation\n");
		return ast_node_stub_new();
	}

	func = (ast_node_func_t *)ast_node_func_def_new(lex_item_prev.name);
	
	ret = process_function_argu(func);
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

	return ast_node_stub_new();
}

static void
func_add_next_argu(ast_node_func_t *func, char *name)
{
	int n;

	n = func->nargs++;

	func->args = realloc_or_die(func->args, 
	    func->nargs * sizeof(*(func->args)));

	func->args[n] = name;
}

//need to flush token stream after errrors
//
static ret_t
process_function_argu(ast_node_func_t *func)
{
	char *id;

	if (match(TOK_LPAR) == FALSE) {
		print_warn("you must set parameter list"
		    "at initialisation\n");
		goto err;	
	}

	//get arguments
	while (match(TOK_RPAR) == FALSE) {
		if (match(TOK_ID) == FALSE) {
			print_warn("identifier required\n");
			goto err;
		}

		id = lex_item_prev.name;

		func_add_next_argu(func, id);

		if (match(TOK_COMMA) == FALSE &&
		    current_tok != TOK_RPAR) {
			print_warn("comma expected");
			goto err;
		}
	}

	return ret_ok;

err:

	if (func->args != NULL) {
		int i;
		for (i = 0; i < func->nargs; i++)
			ufree(func->args[i]);
		ufree(func->args);
	}

	return ret_err;
}

static ret_t
process_function_body(ast_node_func_t *func)
{
	struct syn_ctx *ctx = NULL;
	
	if (match(TOK_LBRACE) == FALSE) {
		print_warn("LBRACE required\n");
		goto err;
	}

	ctx = syn_ctx_new();
	ctx->type = CTX_FUNCTION;

	//FIXME: void body functions
	func->body = stmts(ctx);
	if (nerrors != 0) {
		print_warn("cant traverse func body\n");
		goto err;
	}

	if (match(TOK_RBRACE) == FALSE) {
		print_warn("'}' missing\n");
		goto err;
	}
	
	syn_ctx_free(ctx);

	return ret_ok;

err:

	nerrors++;
	ast_node_unref(func->body);
	func->body = NULL;

	if (ctx != NULL)
		syn_ctx_free(ctx);

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
		node = expr();
		if (node == NULL) {
			print_warn("expected statesment");
			goto err;
		}

		ast_node_func_call_add_next_arg(call, node);

		if (match(TOK_COMMA) == FALSE &&
		    current_tok != TOK_RPAR) {
			print_warn("comma expected");
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
			ind = realloc_or_die(ind, sz * sizeof(*ind));
		}
		
		if (match(TOK_RBRACKET) == FALSE) {
			print_warn("unsupported expression\n");
			goto error;
		}

		ind[dim++] = item;

	} while (match(TOK_LBRACKET));

	//FIXME: now arrays only one-dimention
	
	return ast_node_access_new(name, dim, ind);

error:
	for (i = 0; i < dim; i++)
		ast_node_unref(ind[i]);

	nerrors++;
	return ast_node_stub_new();
}

/*
 * try to init new ast_node_arr_t
 * print warn and return NULL if some error occured
 * return ast_node_arr_t otherwise
 */
 //FIXME: splitme(mb need to add aditional func get_next_arr_item)
static ast_node_t *
array_init()
{
	ast_node_t *item, **arr;

	int i, total, arrsz, ndim, depth;
	int *dimlen, *cnt;
	
	arr = NULL;
	ndim = 0;
	total = arrsz = 0;

	//try to get most depth construction
	while (match(TOK_LBRACE))
		ndim++;
	
	if (ndim == 0)
		print_warn_and_die("this is should not happened\n");

	dimlen = xmalloc(sizeof(dimlen) * ndim);
	memset(dimlen, 0, sizeof(dimlen) * ndim);

	cnt = xmalloc(sizeof(dimlen) * ndim);
	memset(cnt, 0, sizeof(dimlen) * ndim);

	//all except cnt[ndim - 1] must be 1
	for (i = 0; i < ndim - 1; i++)
		cnt[i] = 1;

	depth = ndim;

	do {
		if (match(TOK_RBRACE)) {
			if (cnt[depth - 1] == 0) {
				print_warn("empty scalar initilaizer\n");
				goto error;
			} else if (dimlen[depth - 1] == 0) {
				dimlen[depth - 1] = cnt[depth - 1];
			} else if (dimlen[depth - 1] != cnt[depth - 1]) {
				print_warn("not expected len\n");
				goto error;
			}

			cnt[depth - 1] = 0;

			depth--;
			if (depth == 0)
				break;

			match(TOK_COMMA);
			continue;
		}

		if (match(TOK_LBRACE)) {
			cnt[depth - 1]++;
			depth++;

			if (depth > ndim) {
				print_warn("unexpected depth\n");
				goto error;
			}
			continue;
		}
		//NOTE: values must be on most depth lvl
		if (depth != ndim) {
			print_warn("not expected len\n");
			goto error;
		}
		
		item = logic_or();
		if (item == NULL) {
			print_warn("uncomplited tuple\n");
			goto error;
		}

		if (total >= arrsz) {
			arrsz += 4;
			arr = realloc_or_die(arr, arrsz * sizeof (*arr));
		}

		cnt[depth - 1]++;
		arr[total++] = item;

		if (match(TOK_COMMA) == FALSE && 
		    current_tok != TOK_RBRACE) {
			print_warn("unexpected symbol at arr initialisation\n");
			goto error;
		}
	} while (1);

	if (total == 0) {
		print_warn("empty scalar initializer\n");
		goto error;
	}

	ufree(cnt);
	return ast_node_arr_new(arr, ndim, dimlen, total);

error:
	sync_stream();

	for (i = 0; i < total; i++)
		ast_node_unref(arr[i]);
	
	ufree(dimlen);
	ufree(cnt);
	ufree(arr);
	return NULL;
}

