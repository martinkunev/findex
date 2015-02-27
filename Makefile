export CC=gcc
export CFLAGS=-std=c99 -pthread -g -DOS_LINUX -DOS_BSD -D_GNU_SOURCE -D_BSD_SOURCE -Wno-parentheses -Wno-empty-body
export LDFLAGS=-pthread

all: findex ffind

findex: buffer.o magic.o findex.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o findex

ffind: ffind.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o ffind

install: all
	cp findex /usr/local/bin/
	cp ffind /usr/local/bin/

clean:
	rm -f *.o
	rm -f findex ffind
