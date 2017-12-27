CFLAGS:=$(CFLAGS) -I../ -DOS_LINUX

all: findex ffind

findex: findex.o magic.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

ffind: ffind.o format.o path.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o
	rm -f findex ffind
