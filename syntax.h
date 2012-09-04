#ifndef __SYNTAX_H__
#define __SYNTAX_H__

#include "common.h"
#include "syn_tree.h"


/*
 * Flag, sets if TOK_EOF occured in input 
 * Maybe need to replace with syntax_ctx later.
 */
int syntax_is_eof;


/*
 * Build syntax tree based on input statements.
 */
ret_t program_start(ast_node_t **tree);

#endif

