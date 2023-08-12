CC = g++
RM = rm -f

SRCS   := libtest
TARGETS := $(SRCS)

.PHONY: all clean test $(TARGETS)
all: $(TARGETS)

test: all
	@echo "\033[0;32mOK\033[0;39m"

libtest: libtest.cc
	$(CC) -std=c++14 -c libtest.cc
	$(CC) -o $@ libtest.o ../src/libmds.a
	./$@

clean:
	$(RM) $(TARGETS) *.o
