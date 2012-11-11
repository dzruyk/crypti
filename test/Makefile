include ../Common.mk

STR_OBJS = str_test.o $(patsubst %,../src/%, str.o buffer.o common.o)
STR_TEST = str_test

VAROP_OBJS = var_op_test.o
VAROP_TEST = varop_test

INCLUDES= -I ../include -I ../lib
LIBS= ../lib/libmp.a
LEX_OBJS = $(patsubst %,../src/%, lex.o id_table.o hash.o primes.o \
	keyword.o common.o array.o str.o octstr.o buffer.o type_convertions.o variable.o var_op.o)


all: $(STR_TEST) $(VAROP_TEST)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(STR_TEST): $(STR_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(VAROP_TEST): $(VAROP_OBJS) $(LEX_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES)  -o $@ $^
	
clean:
	rm -f *.o $(STR_TEST) $(VAROP_TEST)

