CC ?= gcc
CFLAGS ?= -O2 -march=native -mtune=native -DNDEBUG
CFLAGS += -Wall -Wextra -Wpedantic -Werror

EXE  := fuzzer points stress

LIBS :=

HDRS := rbtree.h
SRCS := rbtree.c

OBJS := $(SRCS:.c=.o)


.PHONY: all
all: $(EXE)

fuzzer: fuzzer.c $(HDRS) $(OBJS) Makefile
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< $(OBJS)

points: points.c $(HDRS) $(OBJS) Makefile
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< $(OBJS)

stress: stress.c $(HDRS) $(OBJS) Makefile
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< $(OBJS)

$(OBJS): $(HDRS) $(SRCS) Makefile


.PHONY: clean
clean:
	$(RM) $(OBJS) $(EXE)
