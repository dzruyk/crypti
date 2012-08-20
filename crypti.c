#include <stdio.h>

#include "common.h"
#include "function.h"
#include "id_table.h"
#include "keyword.h"
#include "syntax.h"
#include "syn_tree.h"
#include "traverse.h"

void
print_programme_info()
{
	printf("=======================================\n");
	printf("*Simple language for crypto researches*\n");
	printf("*Now work without libbignum           *\n");
	printf("*but Work In Process                  *\n");
	printf("=======================================\n");
}

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

	print_programme_info();

	initialisation();

	do {
		printf(">>> ");

		ret = program_start(&tree);
		if (tree == NULL || ret != ret_ok)
			continue;
		if (traverse_prog(tree) == ret_ok)
			traverse_result();

	} while (syntax_is_eof != 1);

	deinitialisation();
	
	printf("\n");

	return 0;
}

