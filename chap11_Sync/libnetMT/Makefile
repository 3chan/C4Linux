PROGRAM=libnet.a
OBJS=sock.o token.o log.o jconv.o lockf.o
SRCS=$(OBJS:%.o=%.c)
CFLAGS=-g -Wall -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS=
LDLIBS= -lresolv -lpthread
$(PROGRAM):$(OBJS)
	ar r libnet.a $(OBJS)
