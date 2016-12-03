PROGRAM=wlSyncS
LIBNETDIR=../
IPCDIR=$(HOME)/tokuen/ipc/ipc-3.9.1/
OBJS=main.o param.o
SRCS=$(OBJS:%.o=%.c)
CFLAGS=-Wall -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -I$(LIBNETDIR)/libnetMT/ -I$(IPCDIR)/include
LDFLAGS=-L$(LIBNETDIR)/libnetMT/ -L$(IPCDIR)/lib/Linux-3.2
LDLIBS=-lnet -lpthread -lipc
$(PROGRAM):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LDLIBS)
clean:
	rm -fi *.o *~