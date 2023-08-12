MAKE := make

.PHONY: clean test all

all:
		$(MAKE) -C src

test:
		$(MAKE) -C tests $@

clean:
		$(MAKE) -C src $@
		$(MAKE) -C tests $@
