CC=gcc
CFLAGS=-std=c99 -pedantic -O2 -I../ -D_DEFAULT_SOURCE -DUNIT_TESTING
LDFLAGS=-lcmocka

all: string sort

string: string.o

sort: sort.o

clean:
	rm -f string sort
	rm -f string.o sort.o
