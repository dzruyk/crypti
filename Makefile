CC=gcc
CFLAGS=-Wall -ggdb

LEX_OBJS = lex.o id_table.o hash.o primes.o keyword.o common.o array.o 
LEX_MAIN = lex_test.o
LEX_TEST = lex_test
TRAVERSE_OBJS = stack.o syntax.o syn_tree.o traverse.o eval.o function.o list.o libcall.o
TRAVERSE_MAIN = crypti.o
TRAVERSE_TEST = crypti

all: $(LEX_TEST) $(TRAVERSE_TEST)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

$(LEX_TEST): $(LEX_OBJS) $(LEX_MAIN)
	$(CC) $(CFLAGS) -o $(LEX_TEST) $(LEX_OBJS) $(LEX_MAIN)

$(TRAVERSE_TEST): $(LEX_OBJS) $(TRAVERSE_OBJS) $(TRAVERSE_MAIN)
	$(CC) $(CFLAGS) -o $(TRAVERSE_TEST) $(TRAVERSE_OBJS) $(LEX_OBJS) $(TRAVERSE_MAIN)

clean:
	rm -f *.o
	rm $(TRAVERSE_TEST) $(LEX_TEST)

