CFLAGS := -g -Wall --pedantic-errors -std=c11
SRCS   := $(wildcard *.c)

ROCK  := ../rock
ROCKA := librock.a
DEP   := dep

.PHONY: all clean run test

all: $(ROCK)

run: $(ROCK) $(DEP)
	$(ROCK) ./input.ro

$(ROCK): main.o $(ROCKA)
	$(CC) -o $@ $^

$(ROCKA): $(filter-out main.o, $(SRCS:.c=.o))
	$(AR) -r $@ $^

clean:
	$(RM) $(ROCK) $(ROCKA) *.o $(DEP)

test: $(ROCKA)
	$(MAKE) -C tests $@

$(DEP):
	$(RM) $@
	$(CC) -c -MM $(SRCS) > $@

-include $(DEP)
