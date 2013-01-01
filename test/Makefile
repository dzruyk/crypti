include ../Common.mk

STR_OBJS = str_test.o $(patsubst %,../src/%, str.o buffer.o common.o)
STR_TEST = str_test

OCTSTR_OBJS = octstr_test.o $(patsubst %,../src/%, octstr.o buffer.o common.o)
OCTSTR_TEST = octstr_test

VAROP_OBJS = var_op_test.o
VAROP_TEST = varop_test

TYPECONV_TEST = typeconv_test
TYPECONV_OBJS = type_convertions_test.o

MPLIB_OBJS = mplib_test.o
MPLIB_TEST = mplib_test

INCLUDES= -I ../include -I ../lib
LIBS= -lmpl
LEX_OBJS = $(patsubst %,../src/%, lex.o id_table.o hash.o primes.o \
	keyword.o common.o array.o str.o octstr.o buffer.o type_convertions.o variable.o var_op.o)


all: $(STR_TEST) $(OCTSTR_TEST) $(VAROP_TEST) $(MPLIB_TEST) $(TYPECONV_TEST)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(STR_TEST): $(STR_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(OCTSTR_TEST): $(OCTSTR_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(VAROP_TEST): $(VAROP_OBJS) $(LEX_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES)  -o $@ $^

$(MPLIB_TEST): $(MPLIB_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES)  -o $@ $^

$(TYPECONV_TEST): $(TYPECONV_OBJS) $(LEX_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES)  -o $@ $^

$(LIBS):
	$(MAKE) -C ../../lib lib	
clean:
	rm -f *.o $(STR_TEST) $(VAROP_TEST)

