#ifndef __AST_NODE_H__
#define __AST_NODE_H__

#define AST_NODE(p)((ast_node_t *)p)

typedef enum {
	OP_AS,
	OP_NOT,
	
	OP_EQ,
	OP_NEQ,
	OP_GR,
	OP_LO,
	OP_GE,
	OP_LE,
	OP_L_AND,
	OP_L_OR,
	
	OP_B_AND,
	OP_B_OR,

	OP_PLUS,
	OP_MINUS,
	OP_MUL,
	OP_DIV,

} opcode_t;

typedef enum {
	AST_NODE_AS,
	AST_NODE_OP,
	AST_NODE_SEQ,
	AST_NODE_ARR,
	AST_NODE_ACCESS,
	AST_NODE_DEF,
	AST_NODE_CALL,
	AST_NODE_ID,
	AST_NODE_NUM,
	AST_NODE_RETURN,
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
	int opcode;
} ast_node_op_t;

typedef struct {
	ast_node_t tree;
} ast_node_as_t;

/*
 * freeze
typedef struct {
	ast_node_t tree;
	struct list *nodes;
} ast_node_seq_t;
*/

typedef struct {
	ast_node_t tree;
	int num;
} ast_node_num_t;

typedef struct {
	ast_node_t tree;
	char *name;
} ast_node_id_t;

typedef struct {
	ast_node_t tree;
	ast_node_t **arr;
	int sz;
} ast_node_arr_t;

typedef struct {
	ast_node_t tree;
	char *name;
	ast_node_t *ind;
} ast_node_access_t;

typedef struct {
	ast_node_t tree;
} ast_node_scope_t;

//now body not free with ast_node_unref
typedef struct {
	ast_node_t tree;
	char *name;
	char **args;
	int nargs;
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
	ast_node_t *retval;
} ast_node_return_t;

typedef struct {
	ast_node_t tree;
} ast_node_stub_t;

/*
 * allocate sz bytes for new ast_node with specified type
 * zeroes all structure, set  destructor
 */
ast_node_t *ast_node_new(ast_type_t type, int sz, 
    destructor_t destructor);

ast_node_t *ast_node_num_new(int num);


ast_node_t *ast_node_id_new(char *name);


ast_node_t *ast_node_arr_new(ast_node_t **arr, int sz);

ast_node_t *ast_node_access_new(char *name, ast_node_t *ind);

//WIP

ast_node_t *ast_node_scope_new(ast_node_t *child);

ast_node_t *ast_node_func_def(char *name);

ast_node_t *ast_node_func_call(char *name);
//WIP

ast_node_t *ast_node_op_new(ast_node_t* left, ast_node_t *right, opcode_t opcode);


ast_node_t *ast_node_as_new(ast_node_t *left, ast_node_t *right);


ast_node_t *ast_node_return_new(ast_node_t *retval);


ast_node_t *ast_node_stub_new();


void ast_node_unref(ast_node_t *tree);

#endif

