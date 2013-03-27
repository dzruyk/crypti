include ../../Common.mk

AR=ar
INCLUDES= -I ../../include

OBJS=md5.o sha1.o sha256.o

CRYPT_LIB=crypt_lib.a

all: $(CRYPT_LIB)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(CRYPT_LIB): $(OBJS)
	$(AR) -r $@ $^
clean:
	rm -f *.o
	rm -f *.o
	rm -f $(CRYPT_LIB)
