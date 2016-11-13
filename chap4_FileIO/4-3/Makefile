PROGRAM1=f3-1
PROGRAM2=f3-2
OBJS1=f3-1.o
OBJS2=f3-2.o
SRCS1=$(OBJS1:%.o=%.c)
SRCS2=$(OBJS2:%.o=%.c)
CFLAGS=-Wall -g
LDFLAGS=
LDLIBS=
# make は引数なしで実行すると最初のTARGETのみコンパイルする
# (http://d.hatena.ne.jp/sternheller/20090125/1232863249)
all:$(PROGRAM1) $(PROGRAM2)
$(PROGRAM1):$(OBJS1)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM1) $(OBJS1) $(LDLIBS)
$(PROGRAM2):$(OBJS2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM2) $(OBJS2) $(LDLIBS)
clean:
	rm $(PROGRAM1) $(OBJS1) $(PROGRAM2) $(OBJS2) *~