MAKE := make
BINDIR := /usr/local/bin

.PHONY: all clean run test

all:
	$(MAKE) -C src

run:
	$(MAKE) -C src $@

leakcheck:
	$(MAKE) -C src $@

release: clean
	$(MAKE) -C src OPT="-O2 -DNDEBUG"

install: release
	install -m 755 turbine $(BINDIR)/turbine

uninstall:
	rm -f $(BINDIR)/turbine

test:
	$(MAKE) -C tests $@

clean:
	$(MAKE) -C src $@
	$(MAKE) -C tests $@
