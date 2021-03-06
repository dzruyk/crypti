#ifndef __AST_NODE_H__
#define __AST_NODE_H__

#define AST_NODE(p)((ast_node_t *)p)

typedef enum {
	OP_AS,
	OP_NOT,
	OP_NOTNOT,
	
	OP_EQ,
	OP_NEQ,
	OP_GR,
	OP_LO,
	OP_GE,
	OP_LE,
	OP_L_AND,
	OP_L_OR,
	OP_SHL,
	OP_SHR,
	
	OP_B_AND,
	OP_B_XOR,
	OP_B_OR,

	OP_PLUS,
	OP_MINUS,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_POW,

	OP_STR_CONCAT,
	OP_OCTSTR_CONCAT,

} opcode_t;

typedef enum {
	AST_NODE_VAR,
	AST_NODE_ID,
	AST_NODE_ARR,
	AST_NODE_ACCESS,
	AST_NODE_DEF,
	AST_NODE_CALL,
	AST_NODE_UNARY,
	AST_NODE_TRENARY,
	AST_NODE_OP,
	AST_NODE_OP_AS,
	AST_NODE_AS,
	AST_NODE_SEQ,
	AST_NODE_IF,
	AST_NODE_FOR,
	AST_NODE_WHILE,
	AST_NODE_DO,
	AST_NODE_DEL,
	AST_NODE_BREAK,
	AST_NODE_CONTINUE,
	AST_NODE_RETURN,
	AST_NODE_IMPORT,
	AST_NODE_SCOPE,
	AST_NODE_STUB,
	AST_NODE_UNKNOWN,
} ast_type_t;

struct ast_node;

typedef void (*destructor_t)(struct ast_node *p);

typedef struct ast_node {
	ast_type_t type;
	struct ast_node *left;
	struct ast_node *right;
	struct ast_node *parrent;
	struct ast_node *child;
	destructor_t destructor;
} ast_node_t;	

typedef struct {
	ast_node_t tree;
	struct variable *var;
} ast_node_var_t;

typedef struct {
	ast_node_t tree;
	char *name;
} ast_node_id_t;

typedef struct {
	ast_node_t tree;
	ast_node_t **arr;
	int sz;			//total len of all items
} ast_node_arr_t;

typedef struct {
	ast_node_t tree;
	char *name;
	int dims;
	ast_node_t **ind;
} ast_node_access_t;

typedef struct {
	ast_node_t tree;
	char *name;
	char **args;
	int nargs;
	char **retargs;
	int nret;
	ast_node_t *body;
} ast_node_func_t;

typedef struct {
	ast_node_t tree;
	char *name;
	ast_node_t **args;
	int nargs;
} ast_node_func_call_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *node;
	int opcode;
} ast_node_unary_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *cond;
	ast_node_t *_if_yes;
	ast_node_t *_if_no;
} ast_node_trenary_t;

typedef struct {
	ast_node_t tree;
	int opcode;
} ast_node_op_t;

typedef struct {
	ast_node_t tree;
	int opcode;
} ast_node_op_as_t;

typedef struct {
	ast_node_t tree;
} ast_node_as_t;

typedef struct {
	ast_node_t tree;
	ast_node_t **node;
	int n;
} ast_node_seq_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *_if;
	ast_node_t *body;
	ast_node_t *_else;
} ast_node_if_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *expr1;
	ast_node_t *expr2;
	ast_node_t *expr3;
	ast_node_t *body;
} ast_node_for_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *cond;
	ast_node_t *body;
} ast_node_while_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *cond;
	ast_node_t *body;
} ast_node_do_t;

typedef struct {
	ast_node_t tree;
	ast_node_t *id;
} ast_node_del_t;

typedef struct {
	ast_node_t tree;
} ast_node_break_t;

typedef struct {
	ast_node_t tree;
} ast_node_continue_t;

typedef struct {
	ast_node_t tree;
} ast_node_return_t;

typedef struct {
	ast_node_t tree;
	ast_node_t **nodes;
	int len;
} ast_node_import_t;

typedef struct {
	ast_node_t tree;
} ast_node_scope_t;

typedef struct {
	ast_node_t tree;
} ast_node_stub_t;

/*
 * Allocate sz bytes for new ast_node with specified type
 * zeroes all structure and sets  destructor.
 */
ast_node_t *ast_node_new(ast_type_t type, int sz, 
    destructor_t destructor);

ast_node_t *ast_node_var_new(struct variable *var);

ast_node_t *ast_node_id_new(char *name);

/*
 * This is function not used now.
 * May be it will be implemented sometime in future
 */
//ast_node_t *ast_node_copy(ast_node_t *node);

ast_node_t *ast_node_arr_new(ast_node_t **arr, int sz);


ast_node_t *ast_node_access_new(char *name, int dims, ast_node_t **ind);

ast_node_t *ast_node_func_def_new();

ast_node_t *ast_node_func_call_new(char *name);

void ast_node_func_call_add_next_arg(ast_node_func_call_t *call, ast_node_t *node);

ast_node_t *ast_node_trenary_new(ast_node_t *cond, 
    ast_node_t *_if_yes, ast_node_t *_if_no);

ast_node_t *ast_node_unary_new(ast_node_t *node, opcode_t opcode);

ast_node_t *ast_node_op_new(ast_node_t* left, ast_node_t *right, opcode_t opcode);

ast_node_t *ast_node_op_as_new(ast_node_t* left, ast_node_t *right, opcode_t opcode);

ast_node_t *ast_node_as_new(ast_node_t *left, ast_node_t *right);

ast_node_t *ast_node_seq_new();
void ast_node_seq_add(ast_node_seq_t *seq, ast_node_t *node);

ast_node_t *ast_node_if_new(ast_node_t *_if, ast_node_t *body, ast_node_t *_else);


ast_node_t *ast_node_for_new(ast_node_t *expr1, ast_node_t *expr2,
    ast_node_t *expr3, ast_node_t *body);


ast_node_t *ast_node_while_new(ast_node_t *cond, ast_node_t *body);

ast_node_t *ast_node_do_new(ast_node_t *cond, ast_node_t *body);

ast_node_t *ast_node_del_new(ast_node_t *id);

ast_node_t *ast_node_break_new();

ast_node_t *ast_node_continue_new();

ast_node_t *ast_node_return_new();

ast_node_t *ast_node_import_new(ast_node_t **nodes, int len);

ast_node_t *ast_node_scope_new(ast_node_t *child);

ast_node_t *ast_node_stub_new();

/*
 * If tree is NULL just returns.
 * if not - try to free it.
 */
void ast_node_unref(ast_node_t *tree);

#endif

