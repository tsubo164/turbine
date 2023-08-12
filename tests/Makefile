CC = g++
RM = rm -f

SRCS   := libtest
TARGETS := $(SRCS)

.PHONY: all clean test $(TARGETS)
all: $(TARGETS)

test: all
	@echo "\033[0;32mOK\033[0;39m"

libtest: libtest.cc test.o
	$(CC) -std=c++14 -c libtest.cc
	$(CC) -o $@.out libtest.o test.o ../src/libmds.a
	./$@.out

test.o: test.cc test.h
	$(CC) -std=c++14 -c $<

clean:
	$(RM) $(TARGETS) *.o *.out
