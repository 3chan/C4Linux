PROGRAM=wlSyncS
LIBNETDIR=..
OBJS=main.o param.o
SRCS=$(OBJS:%.o=%.c)
CFLAGS=-Wall -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -I$(LIBNETDIR)/libnetMT/ -I$(HOME)/tokuen/ipc/ipc-3.9.1/include
#CFLAGS=-Wall -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -I$(LIBNETDIR)/libnetMT/
LDFLAGS=-L$(LIBNETDIR)/libnetMT -L$(HOME)/tokuen/ipc/ipc-3.9.1/lib/Linux-3.2
#LDFLAGS=-L$(LIBNETDIR)/libnetMT
LDLIBS=-lnet -lpthread -lipc
#LDLIBS=-lnet -lpthread
$(PROGRAM):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LDLIBS)
