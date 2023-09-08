CC      := g++
AR      := ar
OPT     := -g
CFLAGS  := $(OPT) $(INCLUDE) -Wall --pedantic-errors --std=c++17 -c
RM      := rm -f

SRCS    := ast bytecode codegen interpreter lexer parser scope type vm

.PHONY: all clean run

MDS  := ../mds
MDSA := libmds.a
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS)) main.d

all: $(MDS)

run: $(MDS)
	$(MDS) ./input.md

$(OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $<

main.o: main.cpp
	$(CC) $(CFLAGS) -o $@ $<

$(MDS): main.o $(MDSA)
	$(CC) -o $@ $^

$(MDSA): $(OBJS)
	$(AR) -r $@ $^

clean:
	$(RM) $(MDS) $(MDSA) *.o *.d

test: $(MDSA)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.cpp
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
