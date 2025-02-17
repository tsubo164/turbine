CC = gcc
RM = rm -f

SRCS   := libtest
DEPS   := $(addsuffix .d, $(SRCS))
TARGETS := $(SRCS)

.PHONY: all clean test $(TARGETS)
all: $(TARGETS)

test: all
	@echo "[init dev tests]"
	@../turbine initdev_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[expression tests]"
	@../turbine expr_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[number literal tests]"
	@../turbine numliteral_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[string tests]"
	@../turbine string_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[array tests]"
	@../turbine array_tests.tu foo bar baz
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[map tests]"
	@../turbine map_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[set tests]"
	@../turbine set_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[stack tests]"
	@../turbine stack_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[queue tests]"
	@../turbine queue_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[struct tests]"
	@../turbine struct_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[enum tests]"
	@../turbine enum_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[if statement tests]"
	@../turbine if_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[for statement tests]"
	@../turbine for_tests.tu
	@echo "\033[0;32mOK\033[0;39m"
	@echo "[math module tests]"
	@../turbine math_tests.tu
	@echo "\033[0;32mOK\033[0;39m"

libtest: libtest.out
	@echo "[C tests]"
	@./$@.out
	@echo "\033[0;32mOK\033[0;39m"

libtest.out: libtest.o test.o ../src/libturbine.a
	$(CC) -g -o $@ $^

%.o:%.c
	$(CC) -std=c11 -g -c $<

../src/libturbine.a: ../src/*.h ../src/*.c
	make -C ../src

clean:
	$(RM) $(TARGETS) *.o *.d *.out

$(DEPS): %.d: %.c
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
