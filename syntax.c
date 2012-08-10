#include <stdio.h>

#include <string.h>

#include "common.h"
#include "keyword.h"
#include "macros.h"
#include "syntax.h"
#include "lex.h"
#include "list.h"

/*******SYNTAX_CTX***********/

enum ctx_type {
	CTX_GLOBAL,
	CTX_FUNCTION,
	CTX_CYCLE,
	CTX_UNKNOWN,
};

struct syn_ctx {
	int type;
};

struct syn_ctx *syn_ctx_new();


struct syn_ctx *
syn_ctx_new()
{
	struct syn_ctx *ctx;

	ctx = malloc_or_die(sizeof(*ctx));
	memset(ctx, 0, sizeof(ctx));

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
static ast_node_t *statesment();

static ast_node_t *assign(ast_node_t *lvalue);

static ast_node_t *logic_disj();
static ast_node_t *logic_conj();
static ast_node_t *equity();
static ast_node_t *rel_op();

static ast_node_t *expr();
static ast_node_t *expr_rest();
static ast_node_t *term();
static ast_node_t *term_rest();
static ast_node_t *factor();
static ast_node_t *number();

static ast_node_t *identifier();

static ast_node_t *process_scope();

static ast_node_t *process_function();
static ret_t process_function_argu(ast_node_func_t *func);
static ret_t process_function_body(ast_node_func_t *func);

static ast_node_t *function_call();
static ast_node_t *array_access();
static ast_node_t *array_init();

extern struct lex_item lex_item;
extern struct lex_item lex_item_prev;
extern tok_t current_tok;


int syntax_is_eof = 0;

static int nerrors;

struct lex_item lex_item_prev;
tok_t current_tok;


static void
update_prev_token()
{
	lex_item_prev.id = lex_item.id;
	switch(lex_item.id) {
	case TOK_ID:
		lex_item_prev.name = lex_item.name;
		break;
	case TOK_NUM:
		lex_item_prev.num = lex_item.num;
		break;
	default:
		lex_item_prev.op = lex_item.op;
	}
}

static void
tok_next()
{
	update_prev_token();
	current_tok = get_next_token();
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

static inline void
sync_stream()
{
	while (current_tok != TOK_EOL &&
	    current_tok != TOK_EOF &&
	    current_tok != TOK_SEMICOLON) {

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
	nerrors = 0;
	*tree = NULL;
	
	tok_next();

	*tree = global_expr();
	
	if (nerrors != 0) {
		//now we must flush tree
		if (*tree != NULL) {
			ast_node_unref(*tree);
			*tree = NULL;
		}
		return ret_err;
	}

	return ret_ok;
}

static ast_node_t *
global_expr()
{
	ast_node_t *node;
	struct syn_ctx *ctx;

	//Define new function
	if (match(TOK_DEF))
		return process_function();

	ctx = syn_ctx_new();
	ctx->type = CTX_GLOBAL;

	node = stmts(ctx);

	syn_ctx_free(ctx);

	return node;
}

static ast_node_t *
stmts(struct syn_ctx *ctx)
{
	
	if (match(TOK_LBRACE)) 
		return process_scope();

	if (ctx->type != CTX_GLOBAL)
		return block(ctx);
	
	return statesment();
}

static ast_node_t *
block(struct syn_ctx *ctx)
{
	ast_node_t *result, *prev, *tmp;

	result = prev = NULL;

	//FIXME: bad, bad cycle
	while (is_eof() != TRUE) {

		//end of scope
		if (current_tok == TOK_RBRACE
		    && (ctx->type == CTX_FUNCTION 
		    || ctx->type == CTX_CYCLE))
			break;
	
		tmp = statesment();

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

		if (is_stmt_end() == TRUE) {
			tok_next();
		} else {
			nerrors++;
			sync_stream();
			print_warn("expected end of statesment\n");
			break;
		}

		//FIXME: rewrite me
		if (ctx->type == CTX_GLOBAL)
			break;

		if (tmp == NULL)
			continue;
	}

	return result;
}

//FIXME: REWRITEME!
static ast_node_t *
statesment()
{
	ast_node_t *result;

	result = logic_disj();
	if (result == NULL)
		return NULL;

	if (match(TOK_AS) == TRUE)
		return assign(result);
	
	return result;
}

static ast_node_t *
assign(ast_node_t *lvalue)
{
	ast_node_t *right = NULL;
	
	//FIXME: mb need to write special function at future?
	if (lvalue->type != AST_NODE_ID &&
	    lvalue->type != AST_NODE_ACCESS) {
		print_warn("assign to not variable\n");
		goto err;
	}

	//to array
	if (match(TOK_LBRACE)) {
		right = array_init();

		if (match(TOK_RBRACE) == FALSE) {
			print_warn("right bracket missed\n");
			goto err;
		}

		return ast_node_as_new(lvalue, right);
	}

	//to number

	right = statesment();
	if (right == NULL) {
		print_warn("uncomplited as expression\n");
		goto err;
	}

	return ast_node_as_new(lvalue, right);

	err:
		nerrors++;
		if (right != NULL)
			ast_node_unref(right);
		ast_node_unref(lvalue);
		return ast_node_stub_new();

}

static ast_node_t *
logic_disj()
{
	ast_node_t *right, *result;

	result = logic_conj();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		if (match(TOK_L_OR) == FALSE)
			return result;

		right = logic_conj();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited eq expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_L_OR);
	}
	return result;

}

static ast_node_t *
logic_conj()
{
	ast_node_t *right, *result;

	result = equity();

	if (result == NULL)
		return NULL;
	
	while (TRUE) {
		if(match(TOK_L_AND) == FALSE)
			return result;
		
		right = equity();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited eq expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, OP_L_AND);
	}
	return result;
}

static ast_node_t *
equity()
{
	ast_node_t *right, *result;
	opcode_t op;

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

	result = expr();

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

		right = expr();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited rel expression\n");
			right = ast_node_stub_new();
		}
		result = ast_node_op_new(result, right, op);
	}
}


static ast_node_t *
expr()
{
	ast_node_t *tree;

	tree = term();

	if (tree == NULL)
		return NULL;
	
	return expr_rest(tree);
}

ast_node_t *
expr_rest(ast_node_t *left)
{
	ast_node_t *right, *result;
	int op;

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

		right = term();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited expr expression\n");
			right = ast_node_stub_new();
		}

		result = ast_node_op_new(result, right, op);
	}
}

static ast_node_t *
term()
{
	ast_node_t *tree;

	tree = factor();
	if (tree == NULL) 
		return NULL;

	return term_rest(tree);
}

static ast_node_t *
term_rest(ast_node_t *left)
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

		right = factor();
		if (right == NULL) {
			nerrors++;
			print_warn("uncomplited term expression\n");
			right = ast_node_stub_new();
		}

		result = ast_node_op_new(result, right, op);
	}
}

// may be need to return ast_node_stub_t when RBRACKET or RPAR missed?
static ast_node_t *
factor()
{
	ast_node_t *stat;
	
	if (match(TOK_ID)) {
		
		return 	identifier();
	
	} else if (match(TOK_RETURN)) {
		
		return ast_node_return_new();
	
	} else if (match(TOK_LPAR)) {
		stat = statesment();
		
		if (match(TOK_RPAR) == FALSE) {
			print_warn("right parenthesis missed\n");
			nerrors++;
			return stat;
		}
		
		return stat;

	} else if (current_tok == TOK_EOL
	    || current_tok == TOK_SEMICOLON
	    || current_tok == TOK_RBRACE) {
		
		return NULL;

	} else if (is_eof() == TRUE) {
		return NULL;
	}
	
	//then we have number
	return number();
}

static ast_node_t *
number()
{
	int sign;
	sign = 1;

	//unary +- implementation
	do {
		if (current_tok == TOK_MINUS)
			sign = -1;
		else if (current_tok == TOK_PLUS)
			sign = 1;
	} while (match(TOK_PLUS) == TRUE || match(TOK_MINUS) == TRUE);

	if (match(TOK_NUM) == FALSE) {
		
		nerrors++;
		print_warn("unsupported token tryed to factor\n");
		tok_next();
		
		return ast_node_stub_new();
	}  

	return ast_node_num_new(lex_item_prev.num * sign);
}

static ast_node_t *
identifier()
{
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
process_scope()
{
	print_warn_and_die("WIP\n");

	return ast_node_stub_new();
}

// FIXME: memory management
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

	func = (ast_node_func_t *)ast_node_func_def(lex_item_prev.name);
	
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
	ast_node_unref((ast_node_t *)func);

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
	int i;

	i = 0;

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
	if (func->body == NULL) {
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
	if (func->body != NULL) {
		ast_node_unref(func->body);
		func->body = NULL;
	}
	if (ctx != NULL)
		syn_ctx_free(ctx);

	return ret_err;
}
		
void
func_call_add_next_argu(ast_node_func_call_t *call, ast_node_t *node)
{
	int n;

	n = call->nargs++;

	call->args = realloc_or_die(call->args,
	    call->nargs * sizeof(*(call->args)));

	call->args[n] = node;

}

static ast_node_t *
function_call()
{
	ast_node_func_call_t *call;
	ast_node_t *node;
	char *name;

	name = lex_item_prev.name;
	call = (ast_node_func_call_t *)ast_node_func_call(name);

	if (match(TOK_LPAR) == FALSE) {
		print_warn("you must set parameter list"
		    "at initialisation\n");
		goto err;
	}

	while (match(TOK_RPAR) == FALSE) {
		
		//get next ast_node
		node = statesment();
		if (node == NULL) {
			print_warn("expected statesment");
			goto err;
		}

		func_call_add_next_argu(call, node);

		if (match(TOK_COMMA) == FALSE &&
		    current_tok != TOK_RPAR) {
			print_warn("comma expected");
			goto err;
		}
	}
	
	return AST_NODE(call);

	err:

	nerrors++;
	ast_node_unref((ast_node_t *)call);
	
	return ast_node_stub_new();
}

static ast_node_t *
array_access()
{
	char *name;
	ast_node_t *ind;

	name = lex_item_prev.name;
	
	tok_next();
	
	ind = logic_disj();
	if (ind == NULL) {
		nerrors++;
		print_warn("null expression in brackets");
		return ast_node_stub_new();
	}
	
	if (match(TOK_RBRACKET) == FALSE) {
		nerrors++;
		print_warn("unsupported expression\n");
		return ast_node_stub_new();
	}

	//FIXME: now arrays only one-dimention
	
	return ast_node_access_new(name, ind);
}

static ast_node_t *
array_init()
{
	ast_node_t *item, **arr;

	int i, len, sz;
	
	arr = NULL;
	len = sz = 0;

	do {
		item = logic_disj();
		if (item == NULL) {
			nerrors++;
			print_warn("uncomplited tuple\n");
			goto error;
		}

		if (len >= sz) {
			sz += 4 * sizeof (*arr);

			arr = realloc_or_die(arr, sz);
		}
		
		arr[len++] = item;

	} while (match(TOK_COMMA) != FALSE);

	return ast_node_arr_new(arr, len);

error:
	for (i = 0; i < len; i++)
		ast_node_unref(arr[i]);
	
	free(arr);
	return ast_node_stub_new();
}

