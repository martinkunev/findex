export CC=gcc
export CFLAGS=-std=c99 -pthread -O2 -DOS_LINUX -DOS_BSD -D_GNU_SOURCE -D_BSD_SOURCE -Wno-parentheses -Wno-empty-body
export LDFLAGS=-pthread

all: findex ffind

findex: findex.o magic.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o findex

ffind: ffind.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o ffind

check:
	$(MAKE) -C test check

clean:
	rm -f *.o
	rm -f findex ffind
	$(MAKE) -C test clean

install: all
	cp findex /usr/local/bin/
	cp ffind /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/findex
	rm -f /usr/local/bin/ffind
