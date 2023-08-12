.PHONY: clean test

mds: main.cc
		g++ -Wall --std=c++14 -o $@ $<

test: mds
		echo 42 > in
		./mds 42 > out
		diff in out
		@echo OK

clean:
		rm -f mds in out
