#ifndef __TRAVERSE_H__
#define __TRAVERSE_H__

#include "common.h"
#include "syn_tree.h"

/*
 * Traverse passed tree, returns:
 * ret_ok if all good);
 * ret_err if something go wrong.
 */

ret_t traverse_prog(ast_node_t *tree);

/*
 * Pop all items from stack and prints it.
 */
void traverse_result();

#endif

