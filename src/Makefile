CC      := g++
AR      := ar
OPT     := -g
CFLAGS  := $(OPT) $(INCLUDE) -Wall --pedantic-errors --std=c++17 -c
RM      := rm -f

SRCS    := ast bytecode codegen error escseq interpreter lexer parser scope type vm

.PHONY: all clean run test

ROCK  := ../rock
ROCKA := librock.a
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS)) main.d

all: $(ROCK)

run: $(ROCK)
	$(ROCK) ./input.ro

$(OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $<

main.o: main.cpp
	$(CC) $(CFLAGS) -o $@ $<

$(ROCK): main.o $(ROCKA)
	$(CC) -o $@ $^

$(ROCKA): $(OBJS)
	$(AR) -r $@ $^

clean:
	$(RM) $(ROCK) $(ROCKA) *.o *.d

test: $(ROCKA)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.cpp
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
