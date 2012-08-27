#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "hash.h"
#include "keyword.h"
#include "lex.h"
#include "macros.h"

#define INITIAL_SZ 32

typedef struct {
	tok_t id;
	char *name;
} keyword_table_item_t;

struct hash_table *key_table;

keyword_table_item_t keywords[] =  {
	{TOK_DEF, "def"},
	{TOK_IF, "if"},
	{TOK_ELSE, "else"},
	{TOK_FOR, "for"},
	{TOK_DO, "do"},
	{TOK_WHILE, "while"},
	{TOK_CONTINUE, "continue"},
	{TOK_BREAK, "break"},
	{TOK_RETURN, "return"},
	{TOK_IMPORT, "import"},
};


static void keyword_table_fill();
static ret_t keyword_table_insert(keyword_table_item_t *item);


static int
keyword_compare(void *a, void *b)
{	
	return strcmp((char*)a, (char*)b);
}

static unsigned long
keyword_hash_cb(const void *data)
{
	int i, mult, res;
	char *s;
	
	mult = 31;
	res = 0;
	s = (char*)data;

	for (i = 0; i < strlen(data); i++)
		res = res * mult + s[i];
	return res;
}

static void
keyword_table_destroy_cb(keyword_table_item_t *item)
{
	free(item);
}

void 
keyword_table_init()
{
	if (key_table != NULL)
		print_warn_and_die("keyword already initialisated!");
	
	key_table = hash_table_new(INITIAL_SZ, 
	    (hash_callback_t )keyword_hash_cb,
	    (hash_compare_t )keyword_compare);
	
	if (key_table == NULL)
		print_warn_and_die("error at table table creation\n");
	
	keyword_table_fill();

}


static void
keyword_table_fill()
{
	int i;
	keyword_table_item_t *tmp;

	if (key_table == NULL)
		print_warn_and_die("key_table uninit\n");
	
	for (i = 0; i < ARRSZ(keywords); i++) {
		tmp = malloc_or_die(sizeof(*tmp));

		tmp->id = keywords[i].id;
		tmp->name = keywords[i].name;

		keyword_table_insert(tmp);
	}
}

static ret_t 
keyword_table_insert(keyword_table_item_t *item)
{
	int res;
	
	res = hash_table_insert_unique(key_table, item->name, item);
	if (res == ret_out_of_memory)
		print_warn_and_die("error at key table insertion\n");
	else if (res == ret_entry_exists)
		print_warn_and_die("internal error, entry exists\n");

	return ret_ok;
}

int
keyword_table_lookup(char *name)
{
	keyword_table_item_t *res;

	if (hash_table_lookup(key_table, name, (void **)&res) != ret_ok)
		return TOK_UNKNOWN;
	else
		return res->id;
}

void 
keyword_table_destroy()
{
	void *key, *data;
	struct hash_table_iter *iter;

	iter = hash_table_iterate_init(key_table);
	
	while (hash_table_iterate(iter, &key, &data) != FALSE)
		keyword_table_destroy_cb((keyword_table_item_t*)data);

	hash_table_iterate_deinit(&iter);

	hash_table_destroy(&key_table);
}

