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

%.o:%.cpp
	$(CC) -std=c++17 -g -c $<

../src/libmds.a: ../src/*.h ../src/*.cpp
	make -C ../src

clean:
	$(RM) $(TARGETS) *.o *.d *.out

$(DEPS): %.d: %.cpp
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
