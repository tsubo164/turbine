MAKE := make

.PHONY: all clean run test

all:
		$(MAKE) -C src

run:
		$(MAKE) -C src $@

test:
		$(MAKE) -C tests $@

clean:
		$(MAKE) -C src $@
		$(MAKE) -C tests $@
