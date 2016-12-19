#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "crypt_hashes.h"
#include "log.h"
#include "eval.h"
#include "function.h"
#include "id_table.h"
#include "keyword.h"
#include "lex.h"
#include "stack.h"
#include "syntax.h"
#include "syn_tree.h"
#include "traverse.h"

void
print_programme_info()
{
	printf("=======================================\n");
	printf("*Simple language for crypto researches*\n");
	printf("=======================================\n");
}

void
print_help_message()
{
	printf(
	"Usage: crypti [-h] [INPUT_FILE]\n"
	"-h print this help message "
	"with no INPUT_FILE or whet INPUT_FILE is - read from "
	"standard input (REPL loop)\n"
	);

}

void
parse_cl_arguments(int argc, char **argv)
{
	FILE *input;
	int opt;

	opt = 0;
	while ((opt = getopt(argc, argv, "h")) != -1) { 
		switch (opt) {
		case 'h':
		default:
			print_help_message();
			exit(0);
		}
	}

	argc -= optind;
	argv = argv + optind;

	if (argc == 0)
		set_input(stdin);
	else {
		DEBUG(LOG_DEFAULT, "input file is %s\n", argv[0]);

		input = fopen(argv[0], "r");
		if (input == NULL) {
			perror("can't open input file:");
			exit(1);
		}

		set_input(input);
	}
}

void
initialisation()
{
	//print_programme_info();

	id_table_init();
	keyword_table_init();
	function_table_init();
	hash_ctx_table_init();
	stack_init((stack_destructor)eval_free);
}

void
deinitialisation()
{
	id_table_destroy();
	keyword_table_destroy();
	function_table_destroy();
	hash_ctx_table_destroy();
	stack_destroy();
}

int
main(int argc, char *argv[])
{
	ast_node_t *tree;
	ret_t ret;

	parse_cl_arguments(argc, argv);

	initialisation();

	do {
		//printf(">>> ");

		ret = program_start(&tree);
		if (tree == NULL || ret != ret_ok)
			continue;
		if (traverse_prog(tree) == ret_ok)
			traverse_result();

	} while (syntax_is_eof != 1);

	deinitialisation();

	//printf("\n");

	return 0;
}

