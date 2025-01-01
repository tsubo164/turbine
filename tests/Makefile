CC = gcc
RM = rm -f

SRCS   := libtest
DEPS   := $(addsuffix .d, $(SRCS))
TARGETS := $(SRCS)

.PHONY: all clean test $(TARGETS)
all: $(TARGETS)

test: all
	@echo "[Rock tests]"
	../rock test_initdev.ro
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[number literal tests]"
	../rock test_numliteral.ro
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[string tests]"
	../rock test_string.ro
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[array tests]"
	../rock test_array.ro foo bar baz
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[struct tests]"
	../rock test_struct.ro
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[for statement tests]"
	../rock test_for.ro
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[math module tests]"
	../rock test_math.ro
	@echo "\033[0;32mOK\033[0;39m"

libtest: libtest.out
	@echo "[C tests]"
	./$@.out
	@echo "\033[0;32mOK\033[0;39m"

libtest.out: libtest.o test.o ../src/librock.a
	$(CC) -g -o $@ $^

%.o:%.c
	$(CC) -std=c11 -g -c $<

../src/librock.a: ../src/*.h ../src/*.c
	make -C ../src

clean:
	$(RM) $(TARGETS) *.o *.d *.out

$(DEPS): %.d: %.c
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
