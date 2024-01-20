CC      := gcc
AR      := ar
OPT     := -g
CFLAGS  := $(OPT) $(INCLUDE) -Wall --pedantic-errors -std=c11 -c
RM      := rm -f

SRCS    := ast builtin bytecode codegen gc error escseq intern interpreter \
           parser scope token type vm

.PHONY: all clean run test

ROCK  := ../rock
ROCKA := librock.a
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS)) main.d

all: $(ROCK)

run: $(ROCK)
	$(ROCK) ./input.ro

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

main.o: main.c
	$(CC) $(CFLAGS) -o $@ $<

$(ROCK): main.o $(ROCKA)
	$(CC) -o $@ $^

$(ROCKA): $(OBJS)
	$(AR) -r $@ $^

clean:
	$(RM) $(ROCK) $(ROCKA) *.o *.d

test: $(ROCKA)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.c
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
