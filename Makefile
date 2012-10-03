CC=gcc
CFLAGS=-Wall -ggdb

BIN= ./bin
SRC= ./src
TEST= ./test
INCLUDES= -I ./include
LEX_OBJS = $(patsubst %,src/%, lex.o id_table.o hash.o primes.o keyword.o common.o array.o str.o)
LEX_MAIN = lex_test.o
LEX_TEST = $(BIN)/lex_test

STRING_OBJS = $(patsubst %,src/%, str.o str_test.o common.o)
STRING_TEST = $(BIN)/string_test

CRYPTI_OBJS = $(patsubst %,src/%,stack.o syntax.o syn_tree.o traverse.o eval.o function.o list.o libcall.o)
CRYPTI_MAIN = crypti.o
CRYPTI_TEST = $(BIN)/crypti

VPATH= ./src

all: $(LEX_TEST) $(CRYPTI_TEST) $(STRING_TEST)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(LEX_TEST): $(LEX_OBJS) $(LEX_MAIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(STRING_TEST): $(STRING_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(CRYPTI_TEST): $(LEX_OBJS) $(CRYPTI_OBJS) $(CRYPTI_MAIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -f *.o
	rm -f $(SRC)/*.o
	rm $(CRYPTI_TEST) $(LEX_TEST)

.PHONY: test
test: $(CRYPTI_TEST)
	bash $(TEST)/calc_test.sh
	bash $(TEST)/memory_test.sh

rebuild: clean $(CRYPTI_TEST)

debug:
	echo $(LEX_OBJS)
