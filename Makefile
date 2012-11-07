CC=gcc
CFLAGS=-Wall -ggdb
MAKE = make

BIN= ./bin
SRC= ./src
TEST= ./test
INCLUDES= -I ./include -I ./lib
LIBS= ./lib/libmp.a
LEX_OBJS = $(patsubst %,src/%, lex.o id_table.o hash.o primes.o \
	keyword.o common.o array.o str.o octstr.o buffer.o type_convertions.o variable.o var_op.o)

LEX_MAIN = lex_test.o
LEX_TEST = $(BIN)/lex_test

CRYPTI_OBJS = $(patsubst %,src/%,stack.o syntax.o syn_tree.o traverse.o eval.o function.o list.o libcall.o)
CRYPTI_MAIN = crypti.o
CRYPTI_TEST = $(BIN)/crypti

STR_OBJS = $(patsubst %,src/%, str_test.o)
STR_TEST = $(BIN)/str_test
VAROP_OBJS = $(patsubst %,src/%, var_op_test.o)
VAROP_TEST = $(BIN)/varop_test

MP_LIB = ./lib/libmp.a

VPATH= ./src

all: $(LEX_TEST) $(CRYPTI_TEST) $(STR_TEST) $(VAROP_TEST) $(MP_LIB)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(LEX_TEST): $(LEX_OBJS) $(LEX_MAIN) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(CRYPTI_TEST): $(LEX_OBJS) $(CRYPTI_OBJS) $(CRYPTI_MAIN) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(STR_TEST): $(STR_OBJS) $(LEX_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(VAROP_TEST): $(VAROP_OBJS) $(LEX_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES)  -o $@ $^
	
$(MP_LIB): 
	$(MAKE) -C ./lib lib
clean:
	rm -f *.o
	rm -f $(SRC)/*.o
	rm $(CRYPTI_TEST) $(LEX_TEST) $(STR_TEST)
	$(MAKE) -C ./lib clean

.PHONY: test
test: $(CRYPTI_TEST)
	bash $(TEST)/calc_test.sh
	bash $(TEST)/memory_test.sh

rebuild: clean $(CRYPTI_TEST)

debug:
	echo $(LEX_OBJS)
