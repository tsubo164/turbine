CFLAGS := -g -Wall --pedantic-errors -std=c11
SRCS   := $(wildcard *.c)

TURBINE  := ../turbine
TURBINEA := libturbine.a
DEP   := dep

.PHONY: all clean run test

all: $(TURBINE)

run: $(TURBINE) $(DEP)
	$(TURBINE) ./input.tu

$(TURBINE): main.o $(TURBINEA)
	$(CC) -o $@ $^

$(TURBINEA): $(filter-out main.o, $(SRCS:.c=.o))
	$(AR) -r $@ $^

clean:
	$(RM) $(TURBINE) $(TURBINEA) *.o $(DEP)

test: $(TURBINEA)
	$(MAKE) -C tests $@

$(DEP):
	$(RM) $@
	$(CC) -c -MM $(SRCS) > $@

-include $(DEP)
