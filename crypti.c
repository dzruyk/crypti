#include <stdio.h>

#include "common.h"
#include "function.h"
#include "id_table.h"
#include "keyword.h"
#include "syntax.h"
#include "syn_tree.h"
#include "traverse.h"

void
initialisation()
{
	id_table_init();
	keyword_table_init();
	function_table_init();
}

void
deinitialisation()
{
	id_table_destroy();
	keyword_table_destroy();
	function_table_destroy();
}

int
main(int argc, char *argv[])
{
	ast_node_t *tree;
	ret_t ret;

	initialisation();

	do {
		ret = program_start(&tree);
		if (tree == NULL || ret != ret_ok)
			continue;
		if (traverse_prog(tree) == ret_ok)
			traverse_result();

	} while (syntax_is_eof != 1);

	deinitialisation();

	return 0;
}

