include Common.mk

BIN= ./bin
SRC= ./src
TEST= ./test/crypti
INCLUDES= -I ./include -I ./lib
LEX_OBJS = $(patsubst %,src/%, lex.o id_table.o hash.o primes.o \
	keyword.o common.o array.o str.o octstr.o buffer.o type_convertions.o variable.o var_op.o)

LEX_MAIN = lex_test.o
LEX_TEST = $(BIN)/lex_test

CRYPT_LIB=lib/crypto/crypt_lib.a

CRYPTI_OBJS = $(patsubst %,src/%,stack.o syntax.o syn_tree.o traverse.o eval.o function.o list.o libcall.o)
CRYPTI_MAIN = crypti.o
CRYPTI_TEST = $(BIN)/crypti

MP_LIB = -lmpl

VPATH= ./src

all: $(LEX_TEST) $(CRYPTI_TEST) $(VAROP_TEST)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(LEX_TEST): $(LEX_OBJS) $(LEX_MAIN) $(MP_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(CRYPTI_TEST): $(LEX_OBJS) $(CRYPTI_OBJS) $(CRYPTI_MAIN) $(MP_LIB) $(CRYPT_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(CRYPT_LIB):
	$(MAKE) -c lib/crypto
clean:
	rm -f *.o
	rm -f $(SRC)/*.o
	rm $(CRYPTI_TEST) $(LEX_TEST) $(STR_TEST)

.PHONY: test
test: $(CRYPTI_TEST)
	$(MAKE) -C ./test
	bash $(TEST)/calc_test.sh
	bash $(TEST)/memory_test.sh

rebuild: clean $(CRYPTI_TEST)

debug:
	echo $(LEX_OBJS)
