CC      := g++
AR      := ar
OPT     := -g
CFLAGS  := $(OPT) $(INCLUDE) -Wall --pedantic-errors --std=c++14 -c
RM      := rm -f

SRCS    := ast bytecode codegen mds parser string_table tokenizer vm

.PHONY: clean

MDS  := libmds.a
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS))

all: $(MDS)

$(OBJS): %.o: %.cc
	$(CC) $(CFLAGS) -o $@ $<

$(MDS): $(OBJS)
	$(AR) -r $@ $^

clean:
	$(RM) $(MDS) *.o *.d

test: $(MDS)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.cc
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
