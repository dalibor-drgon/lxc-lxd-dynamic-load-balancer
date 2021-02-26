
.PHONY: all

all: a.out
	
a.out: src/*.cpp Makefile
	g++ src/*.cpp -o a.out -O2 -llxc -g