CC      := g++
AR      := ar
OPT     := -g
CFLAGS  := $(OPT) $(INCLUDE) -Wall --pedantic-errors --std=c++14 -c
RM      := rm -f

SRCS    := ast bytecode codegen interpreter parser scope string_table \
           tokenizer vm

.PHONY: clean

MDS  := libmds.a
OBJS := $(addsuffix .o, $(SRCS))
DEPS := $(addsuffix .d, $(SRCS))

all: $(MDS)

$(OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $<

$(MDS): $(OBJS)
	$(AR) -r $@ $^

clean:
	$(RM) $(MDS) *.o *.d

test: $(MDS)
	$(MAKE) -C tests $@

$(DEPS): %.d: %.cpp
	$(CC) $(INCLUDE) -c -MM $< > $@

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEPS)
endif
