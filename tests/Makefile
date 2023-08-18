CC = g++
RM = rm -f

SRCS   := libtest
DEPS   := $(addsuffix .d, $(SRCS))
TARGETS := $(SRCS)

.PHONY: all clean test $(TARGETS)
all: $(TARGETS)

test: all
	@echo "\033[0;32mOK\033[0;39m"

libtest: libtest.out
	./$@.out

libtest.out: libtest.o test.o ../src/libmds.a
	$(CC) -o $@ $^

%.o:%.cc
	$(CC) -std=c++14 -g -c $<

../src/libmds.a: ../src/*.h ../src/*cc
	make -C ../src

clean:
	$(RM) $(TARGETS) *.o *.d *.out

$(DEPS): %.d: %.cc
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
