#include <stdio.h>

#include "common.h"
#include "function.h"
#include "keyword.h"
#include "macros.h"
#include "syntax.h"
#include "lex.h"
#include "list.h"

static ast_node_t * global_expr();
static ast_node_t * stmts(boolean_t is_global);
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

static ast_node_t *identifier();

static ast_node_t * process_scope();

static ast_node_t * process_function();
static ret_t process_function_argu(ast_node_func_t *func);
static ret_t process_function_body(ast_node_func_t *func);

static ast_node_t *call_function();
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
	}
	else
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
	//Define new function
	if (current_tok == TOK_KEYWORD &&
	    lex_item.op == KEY_DEF)
		return process_function();

	return stmts(TRUE);
}


static ast_node_t *
stmts(boolean_t is_global)
{
	ast_node_t *result, *prev, *tmp;
	
	if (match(TOK_LBRACE)) 
		return process_scope();

	result = prev = NULL;

	while (is_eof() != TRUE) {
		if (current_tok == TOK_EOL && is_global == FALSE)
			tok_next();
		if (current_tok == TOK_EOL && is_global == TRUE)
			break;
		
		tmp = statesment();

		if (result == NULL)
			result = tmp;
		
		if (prev != NULL) {
			prev->child = tmp;
			tmp->parrent = prev;
		}

		prev = tmp;

		if (nerrors != 0)
			break;

		if (tmp == NULL)
			continue;

		if (is_stmt_end() == FALSE) {
			nerrors++;

			sync_stream();
			print_warn("expected end of statesment\n");
			break;
		}

		//FIXME: rewrite me
		if (is_global == TRUE)
			break;

		//end of scope
		//WARNING: no lbrace check
		//and no guarantee that we open new
		//scope previosly
		if (current_tok == TOK_RBRACE)
			break;
	}
	
	return  result;
}

//FIXME: REWRITEME!
static ast_node_t *
statesment()
{
	ast_node_t *result;

	result = logic_disj();
	if (result == NULL)
		return NULL;

	//if we have some expression, we must try to assign...
	if (match(TOK_AS) == TRUE)
		return assign(result);
	
	return result;
}

static ast_node_t *
assign(ast_node_t *lvalue)
{
	ast_node_t *right;
	
	if (lvalue->type != AST_NODE_ID &&
	    lvalue->type != AST_NODE_ACCESS) {
		nerrors++;
		print_warn("assign to not variable\n");
		ast_node_unref(lvalue);
		return ast_node_stub_new();
	}

	//to array
	if (match(TOK_LBRACE)) {
		right = array_init();
		
		if (match(TOK_RBRACE) == FALSE) {
			print_warn("right bracket missed\n");
			nerrors++;
		}

		return ast_node_as_new(lvalue, right);
	}

	//to number

	right = statesment();
	if (right == NULL) {
		nerrors++;
		print_warn("uncomplited as expression\n");
		ast_node_unref(lvalue);
		return ast_node_stub_new();
	}

	return ast_node_as_new(lvalue, right);
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
	
	} else if (match(TOK_NUM)) {
		
		return ast_node_num_new(lex_item_prev.num);

	}  else if (match(TOK_LPAR)) {
		stat = statesment();
		
		if (match(TOK_RPAR) == FALSE) {
			print_warn("right parenthesis missed\n");
			nerrors++;
			return stat;
		}
		
		return stat;

	} else if (current_tok == TOK_EOL || current_tok == TOK_SEMICOLON) {
		
		return NULL;

	} else if (is_eof() == TRUE) {
		return NULL;
	}

	nerrors++;
	print_warn("unsupported token tryed to factor\n");
	tok_next();
	
	stat = ast_node_stub_new();

	return stat;
}

static ast_node_t *
identifier()
{
	switch (current_tok) {
	case TOK_LBRACKET:
		return array_access();
	case TOK_LPAR:
		return call_function();
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

	tok_next();

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
add_next_argu(struct list *arglst, void *data)
{
	struct list_item *item;

	item = list_item_new(data);

	list_item_add(arglst, item);
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

		add_next_argu(func->args, id);

		if (match(TOK_COMMA) == FALSE &&
		    current_tok != TOK_RPAR) {
			print_warn("comma expected");
			goto err;
		}
	}

	return ret_ok;

	err:

	if (func->args != NULL) {
		list_destroy(&(func->args), ufree);
		printf("debug!: after destroy: func->args = %p\n",
		    func->args);
	}

	return ret_err;
}

static ret_t
process_function_body(ast_node_func_t *func)
{
	if (match(TOK_LBRACE) == FALSE) {
		print_warn("LBRACE required\n");
		goto err;
	}

	//FIXME
	func->body = stmts(FALSE);
	if (func->body == NULL) {
		nerrors++;
		print_warn("cant traverse func body\n");
		goto err;
	}

	if (match(TOK_RBRACE) == FALSE) {
		print_warn("'}' missing\n");
		goto err;
	}
	
	return ret_ok;

	err:

	if (func->body != NULL) {
		ast_node_unref(func->body);
		func->body = NULL;
	}

	return ret_err;
}

static ast_node_t *
call_function()
{
	ast_node_func_call_t *call;
	ast_node_t *node;
	char *name;

	print_warn_and_die("WIP\n");
	
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

		add_next_argu(call->args, node);

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
			ast_node_t **tmp;

			sz += 4 * sizeof (*arr);

			tmp = realloc(arr, sz);
			if (tmp == NULL)
				print_warn_and_die("realloc_err\n");
			arr = tmp;
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

