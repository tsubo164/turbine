OPT    ?= -g -O0

CFLAGS := $(OPT) -Wall --pedantic-errors -std=c11
SRCS   := $(wildcard *.c)

TARGET := ../turbine
LIB    := libturbine.a
DEP    := dep

.PHONY: all clean run test

all: $(TARGET)

run: $(TARGET) $(DEP)
	$(TARGET) ./input.tu

$(TARGET): main.o $(LIB)
	$(CC) -o $@ $^

$(LIB): $(filter-out main.o, $(SRCS:.c=.o))
	$(AR) -r $@ $^

clean:
	$(RM) $(TARGET) $(LIB) *.o $(DEP)

test: $(LIB)
	$(MAKE) -C tests $@

$(DEP):
	$(RM) $@
	$(CC) -c -MM $(SRCS) > $@

-include $(DEP)
