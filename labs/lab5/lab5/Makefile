SRCS = $(wildcard *.c)

PROGS = $(patsubst %.c,%,$(SRCS))

CFLAGS = -O2 -g

all: $(PROGS)

%: %.c

	$(CC) $(CFLAGS) -o $@ $<
