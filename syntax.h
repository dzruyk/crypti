#ifndef __SYNTAX_H__
#define __SYNTAX_H__

#include "common.h"
#include "syn_tree.h"

/*
 * flag, sets if TOK_EOF occured in input 
 * mb need to replace with syntax_ctx later
 */
int syntax_is_eof;


/*
 * build syntax tree based on input statements
 */
ret_t program_start(ast_node_t **tree);

#endif

